#include "vtkSZ3Reader.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h"
#include "vtkImageAlgorithm.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVInformationKeys.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "SZ3Multi/SZ3VersionDispatcher.h"

#include <iostream>

vtkStandardNewMacro(vtkSZ3Reader);

vtkSZ3Reader::vtkSZ3Reader()
{
  this->FileName = nullptr;
  this->DomainDimensions[0] = 0;
  this->DomainDimensions[1] = 0;
  this->DomainDimensions[2] = 0;
  this->UseDoublePrecision = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkSZ3Reader::~vtkSZ3Reader()
{
  this->SetFileName(nullptr);
}

void vtkSZ3Reader::SetDomainDimensions(int x, int y, int z)
{
  this->DomainDimensions[0] = x;
  this->DomainDimensions[1] = y;
  this->DomainDimensions[2] = z;
  this->Modified();
}

void vtkSZ3Reader::GetDomainDimensions(int& x, int& y, int& z)
{
  x = this->DomainDimensions[0];
  y = this->DomainDimensions[1];
  z = this->DomainDimensions[2];
}

int vtkSZ3Reader::RequestData(
  vtkInformation* /*request*/, vtkInformationVector** /*inputVector*/, vtkInformationVector* outputVector)
{

  std::cout << "FileName: " << (this->FileName ? this->FileName : "(none)") << std::endl;
  std::cout << "DomainDimensions: " << this->DomainDimensions[0] << ", " << this->DomainDimensions[1] << ", " << this->DomainDimensions[2] << std::endl;
  std::cout << "DoublePrecision: " << this->UseDoublePrecision << std::endl;

  if (!this->FileName) {
    vtkErrorMacro("A FileName must be specified.");
    return 0;
  }

  if (this->DomainDimensions[0] <= 0 || this->DomainDimensions[1] <= 0 || this->DomainDimensions[2] <= 0) {
    vtkErrorMacro("Dimensions must be greater than zero.");
    return 0;
  }

  std::ifstream file(this->FileName, std::ios::binary);
  if(!file) {
    vtkErrorMacro("Could not open file: " << this->FileName);
    return 0;
  }

  file.seekg(0, std::ios::end);
  size_t compressedSize = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<char> compressedBuffer(compressedSize);
  file.read(compressedBuffer.data(), compressedSize);
  file.close();

  // Read and display version info
  uint32_t version = SZ3_ReadVersion(compressedBuffer.data(), compressedBuffer.size());
  if (version == 0) {
    vtkErrorMacro("Failed to read SZ3 version: " << SZ3_GetLastError());
    return 0;
  }
  std::cout << "SZ3 compressed file version: " << SZ3_VersionToString(version) << std::endl;

  vtkImageData* output = vtkImageData::GetData(outputVector);
  output->SetDimensions(this->DomainDimensions);

  if (this->UseDoublePrecision) {
    this->Decompress<double>(output, compressedBuffer);
  }
  else {
    this->Decompress<float>(output, compressedBuffer);
  }

  return 1;
}

template <typename T>
void vtkSZ3Reader::Decompress(
  vtkImageData* output, std::vector<char>& compressedBuffer)
{
  using VtkArrayT = vtkAOSDataArrayTemplate<T>;
  vtkNew<VtkArrayT> dataArray;

  size_t num = static_cast<size_t>(DomainDimensions[0]) *
               static_cast<size_t>(DomainDimensions[1]) *
               static_cast<size_t>(DomainDimensions[2]);

  dataArray->SetNumberOfComponents(1);
  dataArray->SetNumberOfTuples(num);
  dataArray->SetName("scalar");

  // Decompress directly into the VTK array's buffer
  T* decompressedPtr = static_cast<T*>(dataArray->GetVoidPointer(0));
  int dims[3] = {DomainDimensions[0], DomainDimensions[1], DomainDimensions[2]};

  int result;
  if constexpr (std::is_same_v<T, float>) {
    result = SZ3_DecompressFloat(compressedBuffer.data(), compressedBuffer.size(),
                                  decompressedPtr, num, dims);
  } else {
    result = SZ3_DecompressDouble(compressedBuffer.data(), compressedBuffer.size(),
                                   decompressedPtr, num, dims);
  }

  if (result != 0) {
    vtkErrorMacro("Decompression failed: " << SZ3_GetLastError());
    return;
  }

  output->GetPointData()->AddArray(dataArray);
  output->GetPointData()->SetScalars(dataArray);
}

int vtkSZ3Reader::RequestInformation(
    vtkInformation* /*request*/,
    vtkInformationVector** /*inputVector*/,
    vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Set the whole extent
  int extent[6] = {0, 0, 0, 0, 0, 0};
  extent[1] = this->DomainDimensions[0] > 0 ? this->DomainDimensions[0] - 1 : 0;
  extent[3] = this->DomainDimensions[1] > 0 ? this->DomainDimensions[1] - 1 : 0;
  extent[5] = this->DomainDimensions[2] > 0 ? this->DomainDimensions[2] - 1 : 0;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  // Set default origin and spacing
  double origin[3] = {0.0, 0.0, 0.0};
  double spacing[3] = {1.0, 1.0, 1.0};
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);

  return 1;
}

void vtkSZ3Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "DomainDimensions: " << this->DomainDimensions[0] << ", " << this->DomainDimensions[1] << ", " << this->DomainDimensions[2] << "\n";
  os << indent << "UseDoublePrecision: " << this->UseDoublePrecision << "\n";
}
