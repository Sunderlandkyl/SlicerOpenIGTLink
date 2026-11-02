/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University.

==============================================================================*/

// Tests that vtkIGTLToMRMLVideo links every non-key frame to the frame received
// before it, so that a streaming volume node can decode a run of inter-predicted
// frames back to the last keyframe.
//
// No real video codec is needed. A test-only codec ("TEST") is registered with
// the streaming codec factory: a keyframe carries the raw 8-bit pixels and a
// non-key frame carries per-pixel differences from the previous frame. Decoding
// a non-key frame without its predecessor fails, so the decoded image is only
// correct when the converter linked the frames properly. Frames are decoded
// lazily, after several have been received, to exercise the chain walk in
// vtkStreamingVolumeCodec::DecodeFrame().
//
// Video messages are built by hand and handed to an igtlioVideoDevice, so no
// network connection or OpenIGTLink encoder is involved.

// OpenIGTLink includes
#include <igtlVideoMessage.h>

// OpenIGTLinkIO includes
#include <igtlioVideoDevice.h>

// OpenIGTLinkIF MRML includes
#include "vtkIGTLIOConverterFactory.h"
#include "vtkIGTLToMRMLVideo.h"
#include "vtkMRMLIGTLConnectorNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLStreamingVolumeNode.h>
#include "vtkMRMLCoreTestingMacros.h"

// vtkAddon includes
#include <vtkStreamingVolumeCodec.h>
#include <vtkStreamingVolumeCodecFactory.h>
#include <vtkStreamingVolumeFrame.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>

// STD includes
#include <cstring>
#include <vector>

namespace
{

const char* TEST_FOURCC = "TEST";
const int WIDTH = 4;
const int HEIGHT = 3;
const int NUMBER_OF_PIXELS = WIDTH * HEIGHT;

//----------------------------------------------------------------------------
/// Test-only codec. Keyframe payload: raw pixels. Non-key payload: byte-wise
/// difference from the previously decoded image (mod 256).
class vtkTestDeltaVolumeCodec : public vtkStreamingVolumeCodec
{
public:
  static vtkTestDeltaVolumeCodec* New();
  vtkStreamingVolumeCodec* CreateCodecInstance() override;
  vtkTypeMacro(vtkTestDeltaVolumeCodec, vtkStreamingVolumeCodec);

  std::string GetFourCC() override { return TEST_FOURCC; }

  /// Number of frames this codec instance has decoded (for checking the chain walk).
  int DecodeCount = 0;

protected:
  vtkTestDeltaVolumeCodec() = default;
  ~vtkTestDeltaVolumeCodec() override = default;

  bool DecodeFrameInternal(vtkStreamingVolumeFrame* inputFrame, vtkImageData* outputImageData, bool saveDecodedImage) override
  {
    if (!inputFrame || !inputFrame->GetFrameData())
    {
      vtkErrorMacro("Invalid frame");
      return false;
    }
    int dims[3] = { 0, 0, 0 };
    inputFrame->GetDimensions(dims);
    size_t numberOfBytes = static_cast<size_t>(dims[0]) * dims[1] * dims[2] * inputFrame->GetNumberOfComponents();
    if (inputFrame->GetVTKScalarType() != VTK_UNSIGNED_CHAR
      || static_cast<size_t>(inputFrame->GetFrameData()->GetNumberOfValues()) != numberOfBytes)
    {
      vtkErrorMacro("Unexpected frame layout");
      return false;
    }
    const unsigned char* payload = inputFrame->GetFrameData()->GetPointer(0);

    if (inputFrame->IsKeyFrame())
    {
      this->LastImage.assign(payload, payload + numberOfBytes);
    }
    else
    {
      if (this->LastImage.size() != numberOfBytes)
      {
        vtkErrorMacro("Cannot decode a non-key frame without its predecessor");
        return false;
      }
      for (size_t i = 0; i < numberOfBytes; ++i)
      {
        this->LastImage[i] = static_cast<unsigned char>(this->LastImage[i] + payload[i]);
      }
    }
    this->DecodeCount++;

    if (saveDecodedImage)
    {
      int imageDims[3] = { 0, 0, 0 };
      outputImageData->GetDimensions(imageDims);
      if (imageDims[0] != dims[0] || imageDims[1] != dims[1] || imageDims[2] != dims[2]
        || outputImageData->GetScalarType() != VTK_UNSIGNED_CHAR
        || outputImageData->GetNumberOfScalarComponents() != inputFrame->GetNumberOfComponents())
      {
        vtkErrorMacro("Output image does not match frame");
        return false;
      }
      memcpy(outputImageData->GetScalarPointer(), this->LastImage.data(), numberOfBytes);
    }
    return true;
  }

  bool EncodeImageDataInternal(vtkImageData* vtkNotUsed(inputImageData), vtkStreamingVolumeFrame* vtkNotUsed(outputFrame), bool vtkNotUsed(forceKeyFrame)) override
  {
    vtkErrorMacro("Encoding is not supported by the test codec");
    return false;
  }

  bool UpdateParameterInternal(std::string vtkNotUsed(parameterName), std::string vtkNotUsed(parameterValue)) override { return false; }
  std::string GetParameterDescription(std::string vtkNotUsed(parameterName)) override { return ""; }

private:
  std::vector<unsigned char> LastImage;

  vtkTestDeltaVolumeCodec(const vtkTestDeltaVolumeCodec&) = delete;
  void operator=(const vtkTestDeltaVolumeCodec&) = delete;
};
vtkCodecNewMacro(vtkTestDeltaVolumeCodec);

//----------------------------------------------------------------------------
typedef std::vector<unsigned char> Pixels;

Pixels MakePixels(unsigned char start, unsigned char step)
{
  Pixels p(NUMBER_OF_PIXELS);
  for (int i = 0; i < NUMBER_OF_PIXELS; ++i)
  {
    p[i] = static_cast<unsigned char>(start + i * step);
  }
  return p;
}

Pixels Delta(const Pixels& from, const Pixels& to)
{
  Pixels d(NUMBER_OF_PIXELS);
  for (int i = 0; i < NUMBER_OF_PIXELS; ++i)
  {
    d[i] = static_cast<unsigned char>(to[i] - from[i]);
  }
  return d;
}

//----------------------------------------------------------------------------
/// Build a VIDEO message carrying \p payload and set it as the device content,
/// the way igtlioVideoDevice::ReceiveIGTLMessage would after a real receive.
void ReceiveFrame(igtlioVideoDevice* device, const Pixels& payload, bool keyFrame)
{
  igtl::VideoMessage::Pointer message = igtl::VideoMessage::New();
  message->SetHeaderVersion(IGTL_HEADER_VERSION_1);
  message->SetDeviceName(device->GetDeviceName().c_str());
  message->SetWidth(WIDTH);
  message->SetHeight(HEIGHT);
  message->SetAdditionalZDimension(1);
  message->SetFrameType(keyFrame ? igtl::FrameTypeKey : igtl::FrameTypeInterPrediction);
  message->SetBitStreamSize(static_cast<int>(payload.size()));
  message->AllocateScalars();
  memcpy(message->GetPackFragmentPointer(2), payload.data(), payload.size());
  message->Pack();

  vtkSmartPointer<vtkUnsignedCharArray> frameData = vtkSmartPointer<vtkUnsignedCharArray>::New();
  frameData->SetNumberOfValues(static_cast<vtkIdType>(payload.size()));
  memcpy(frameData->GetPointer(0), payload.data(), payload.size());

  igtlioVideoConverter::ContentData content = device->GetContent();
  content.videoMessage = message;
  content.frameData = frameData;
  content.frameType = keyFrame ? igtl::FrameTypeKey : igtl::FrameTypeInterPrediction;
  content.keyFrameUpdated = keyFrame;
  content.grayscale = true;
  strncpy(content.codecName, TEST_FOURCC, IGTL_VIDEO_CODEC_NAME_SIZE);
  if (!content.transform)
  {
    content.transform = vtkSmartPointer<vtkMatrix4x4>::New();
  }
  device->SetContent(content);
}

//----------------------------------------------------------------------------
int CheckImage(vtkMRMLStreamingVolumeNode* node, const Pixels& expected)
{
  vtkImageData* image = node->GetImageData(); // decodes lazily
  CHECK_NOT_NULL(image);
  int dims[3] = { 0, 0, 0 };
  image->GetDimensions(dims);
  CHECK_INT(dims[0], WIDTH);
  CHECK_INT(dims[1], HEIGHT);
  CHECK_INT(dims[2], 1);
  CHECK_INT(image->GetScalarType(), VTK_UNSIGNED_CHAR);
  CHECK_INT(image->GetNumberOfScalarComponents(), 1);
  const unsigned char* actual = static_cast<const unsigned char*>(image->GetScalarPointer());
  for (int i = 0; i < NUMBER_OF_PIXELS; ++i)
  {
    if (actual[i] != expected[i])
    {
      std::cerr << "Pixel " << i << ": expected " << int(expected[i]) << " got " << int(actual[i]) << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

/// Length of the previous-frame chain ending at \p frame, counting \p frame itself.
int ChainLength(vtkStreamingVolumeFrame* frame)
{
  int n = 0;
  for (; frame; frame = frame->GetPreviousFrame())
  {
    ++n;
  }
  return n;
}

} // namespace

//----------------------------------------------------------------------------
int vtkIGTLToMRMLVideoFrameChainTest(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkIGTLIOConverterFactory::GetInstance()->RegisterConverter(vtkSmartPointer<vtkIGTLToMRMLVideo>::New());
  vtkStreamingVolumeCodecFactory::GetInstance()->RegisterStreamingCodec(vtkSmartPointer<vtkTestDeltaVolumeCodec>::New());
  CHECK_NOT_NULL(vtkSmartPointer<vtkStreamingVolumeCodec>::Take(
    vtkStreamingVolumeCodecFactory::GetInstance()->CreateCodecByFourCC(TEST_FOURCC)).GetPointer());

  vtkNew<vtkMRMLScene> scene;
  vtkSmartPointer<vtkMRMLIGTLConnectorNode> connector = vtkSmartPointer<vtkMRMLIGTLConnectorNode>::New();
  scene->AddNode(connector);

  vtkIGTLToMRMLBase* converter = vtkIGTLIOConverterFactory::GetInstance()->GetConverter("VIDEO");
  CHECK_NOT_NULL(converter);

  vtkSmartPointer<igtlioVideoDevice> device = vtkSmartPointer<igtlioVideoDevice>::New();
  device->SetDeviceName("TestVideo");

  const Pixels a = MakePixels(10, 3);
  const Pixels b = MakePixels(50, 5);
  const Pixels c = MakePixels(200, 7);
  const Pixels d = MakePixels(0, 11);
  const Pixels e = MakePixels(99, 1);

  // Keyframe: the converter creates the node from the first message.
  ReceiveFrame(device, a, true);
  vtkMRMLStreamingVolumeNode* node = vtkMRMLStreamingVolumeNode::SafeDownCast(converter->CreateNode(scene, device, connector));
  CHECK_NOT_NULL(node);
  CHECK_STRING(node->GetName(), "TestVideo");
  converter->DeviceToMRML(device, node, connector);
  CHECK_NOT_NULL(node->GetFrame());
  CHECK_BOOL(node->GetFrame()->IsKeyFrame(), true);
  CHECK_NULL(node->GetFrame()->GetPreviousFrame());
  CHECK_STD_STRING(node->GetFrame()->GetCodecFourCC(), TEST_FOURCC);
  CHECK_STD_STRING(node->GetCodecFourCC(), TEST_FOURCC);

  // Two inter-predicted frames, not decoded in between.
  ReceiveFrame(device, Delta(a, b), false);
  converter->DeviceToMRML(device, node, connector);
  ReceiveFrame(device, Delta(b, c), false);
  converter->DeviceToMRML(device, node, connector);

  vtkStreamingVolumeFrame* frameC = node->GetFrame();
  CHECK_NOT_NULL(frameC);
  CHECK_BOOL(frameC->IsKeyFrame(), false);
  CHECK_INT(ChainLength(frameC), 3);
  CHECK_NOT_NULL(frameC->GetPreviousFrame());
  CHECK_BOOL(frameC->GetPreviousFrame()->IsKeyFrame(), false);
  CHECK_NOT_NULL(frameC->GetPreviousFrame()->GetPreviousFrame());
  CHECK_BOOL(frameC->GetPreviousFrame()->GetPreviousFrame()->IsKeyFrame(), true);

  // Decoding frame C must walk back through B to keyframe A.
  CHECK_EXIT_SUCCESS(CheckImage(node, c));
  vtkTestDeltaVolumeCodec* codec = vtkTestDeltaVolumeCodec::SafeDownCast(node->GetCodec());
  CHECK_NOT_NULL(codec);
  CHECK_INT(codec->DecodeCount, 3);

  // Next inter-predicted frame: only the new frame needs decoding.
  ReceiveFrame(device, Delta(c, d), false);
  converter->DeviceToMRML(device, node, connector);
  CHECK_INT(ChainLength(node->GetFrame()), 4);
  CHECK_EXIT_SUCCESS(CheckImage(node, d));
  CHECK_INT(codec->DecodeCount, 4);

  // A new keyframe starts a fresh chain.
  ReceiveFrame(device, e, true);
  converter->DeviceToMRML(device, node, connector);
  CHECK_BOOL(node->GetFrame()->IsKeyFrame(), true);
  CHECK_NULL(node->GetFrame()->GetPreviousFrame());
  CHECK_EXIT_SUCCESS(CheckImage(node, e));
  CHECK_INT(codec->DecodeCount, 5);

  // The connector node holds no per-device frame state; a second node from a
  // different device must get an independent chain.
  vtkSmartPointer<igtlioVideoDevice> device2 = vtkSmartPointer<igtlioVideoDevice>::New();
  device2->SetDeviceName("OtherVideo");
  ReceiveFrame(device2, b, true);
  vtkMRMLStreamingVolumeNode* node2 = vtkMRMLStreamingVolumeNode::SafeDownCast(converter->CreateNode(scene, device2, connector));
  CHECK_NOT_NULL(node2);
  CHECK_BOOL(node2 != node, true);
  converter->DeviceToMRML(device2, node2, connector);
  ReceiveFrame(device2, Delta(b, a), false);
  converter->DeviceToMRML(device2, node2, connector);
  CHECK_INT(ChainLength(node2->GetFrame()), 2);
  CHECK_EXIT_SUCCESS(CheckImage(node2, a));
  // The first node is unaffected.
  CHECK_EXIT_SUCCESS(CheckImage(node, e));

  vtkStreamingVolumeCodecFactory::GetInstance()->UnRegisterStreamingCodecByClassName("vtkTestDeltaVolumeCodec");
  std::cout << "Test passed." << std::endl;
  return EXIT_SUCCESS;
}
