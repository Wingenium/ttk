#include <ttkMandatoryCriticalPoints.h>

#include <vtkCellData.h>
#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>

#include <array>

vtkStandardNewMacro(ttkMandatoryCriticalPoints);

ttkMandatoryCriticalPoints::ttkMandatoryCriticalPoints() {
  SetNumberOfInputPorts(1);
  SetNumberOfOutputPorts(6);
}

int ttkMandatoryCriticalPoints::FillInputPortInformation(int port,
                                                         vtkInformation *info) {
  if(port == 0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
  }
  return 0;
}

int ttkMandatoryCriticalPoints::FillOutputPortInformation(
  int port, vtkInformation *info) {
  if(port >= 0 && port < 4) {
    info->Set(ttkAlgorithm::SAME_DATA_TYPE_AS_INPUT_PORT(), 0);
    return 1;
  } else if(port == 4 || port == 5) {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
    return 1;
  }
  return 0;
}

void buildVtkTree(
  vtkUnstructuredGrid *outputTree,
  ttk::MandatoryCriticalPoints::TreeType treeType,
  const ttk::Graph &graph,
  const std::vector<double> &xCoord,
  const std::vector<double> &yCoord,
  const std::vector<int> &mdtTreePointComponentId,
  const std::vector<ttk::MandatoryCriticalPoints::PointType> &mdtTreePointType,
  const std::vector<double> &mdtTreePointLowInterval,
  const std::vector<double> &mdtTreePointUpInterval,
  const std::vector<int> &mdtTreeEdgeSwitchable) {

  const int numberOfPoints = graph.getNumberOfVertices();
  const int numberOfEdges = graph.getNumberOfEdges();

  /* Point data */
  vtkNew<vtkIntArray> outputTreePointType{};
  outputTreePointType->SetName("Type");
  outputTreePointType->SetNumberOfTuples(numberOfPoints);
  outputTree->GetPointData()->AddArray(outputTreePointType);

  vtkNew<vtkDoubleArray> outputTreePointLowInterval{};
  outputTreePointLowInterval->SetName("LowInterval");
  outputTreePointLowInterval->SetNumberOfTuples(numberOfPoints);
  outputTree->GetPointData()->AddArray(outputTreePointLowInterval);

  vtkNew<vtkDoubleArray> outputTreePointUpInterval{};
  outputTreePointUpInterval->SetName("UpInterval");
  outputTreePointUpInterval->SetNumberOfTuples(numberOfPoints);
  outputTree->GetPointData()->AddArray(outputTreePointUpInterval);

  vtkNew<vtkIntArray> outputTreePointComponentId{};
  outputTreePointComponentId->SetName("ComponentId");
  outputTreePointComponentId->SetNumberOfTuples(numberOfPoints);
  outputTree->GetPointData()->AddArray(outputTreePointComponentId);

  for(int i = 0; i < numberOfPoints; i++) {
    outputTreePointType->SetTuple1(i, static_cast<int>(mdtTreePointType[i]));
    outputTreePointLowInterval->SetTuple1(i, mdtTreePointLowInterval[i]);
    outputTreePointUpInterval->SetTuple1(i, mdtTreePointUpInterval[i]);
    outputTreePointComponentId->SetTuple1(i, mdtTreePointComponentId[i]);
  }

  /* Cell data */
  vtkNew<vtkIntArray> outputTreeEdgeSwitchable{};
  outputTreeEdgeSwitchable->SetName("Switchable");
  outputTreeEdgeSwitchable->SetNumberOfTuples(numberOfEdges);
  outputTree->GetCellData()->AddArray(outputTreeEdgeSwitchable);
  for(int i = 0; i < numberOfEdges; i++) {
    outputTreeEdgeSwitchable->SetTuple1(i, mdtTreeEdgeSwitchable[i]);
  }

  /* New Objects */
  // Points
  vtkNew<vtkPoints> mdtTreePoints{};
  outputTree->SetPoints(mdtTreePoints);
  for(int i = 0; i < numberOfPoints; i++) {
    mdtTreePoints->InsertNextPoint(xCoord[i], yCoord[i], 0.0);
  }

  // Edges (cells)
  outputTree->Allocate(numberOfEdges);
  for(int i = 0; i < numberOfEdges; i++) {
    std::array<vtkIdType, 2> edge{graph.getEdge(i).getVertexIdx().first,
                                  graph.getEdge(i).getVertexIdx().second};
    outputTree->InsertNextCell(VTK_LINE, 2, edge.data());
  }
}

int ttkMandatoryCriticalPoints::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector) {

  const auto input = vtkDataSet::GetData(inputVector[0]);
  auto outputMinimum = vtkDataSet::GetData(outputVector, 0);
  auto outputJoinSaddle = vtkDataSet::GetData(outputVector, 1);
  auto outputSplitSaddle = vtkDataSet::GetData(outputVector, 2);
  auto outputMaximum = vtkDataSet::GetData(outputVector, 3);
  auto outputJoinTree = vtkUnstructuredGrid::GetData(outputVector, 4);
  auto outputSplitTree = vtkUnstructuredGrid::GetData(outputVector, 5);

  // Check the last modification of the input
  if(inputMTime_ != input->GetMTime()) {
    inputMTime_ = input->GetMTime();
    computeAll_ = true;
  }

  // Use a pointer-base copy for the input data
  outputMinimum->ShallowCopy(input);
  outputJoinSaddle->ShallowCopy(input);
  outputSplitSaddle->ShallowCopy(input);
  outputMaximum->ShallowCopy(input);

  // Input data arrays
  vtkDataArray *inputLowerBoundField = nullptr;
  vtkDataArray *inputUpperBoundField = nullptr;

  // Get the upper bound field array in the input data set
  if(upperBoundFiledName_.length()) {
    inputUpperBoundField
      = input->GetPointData()->GetArray(upperBoundFiledName_.c_str());
  } else {
    inputUpperBoundField = input->GetPointData()->GetArray(upperBoundId);
  }
  // Error if not found
  if(!inputUpperBoundField) {
    return -1;
  }

  // Get the lower bound field array in the input data set
  if(lowerBoundFieldName_.length()) {
    inputLowerBoundField
      = input->GetPointData()->GetArray(lowerBoundFieldName_.c_str());
  } else {
    inputLowerBoundField = input->GetPointData()->GetArray(lowerBoundId);
  }
  // Error if not found
  if(!inputLowerBoundField) {
    return -1;
  }

  this->printMsg("Using `" + std::string{inputLowerBoundField->GetName()}
                 + "' as lower bound...");
  this->printMsg("Using `" + std::string{inputUpperBoundField->GetName()}
                 + "' as upper bound...");

  // Initialize the triangulation object with the input data set
  auto triangulation = ttkAlgorithm::GetTriangulation(input);

  if(triangulation == nullptr)
    return -1;

  this->preconditionTriangulation(triangulation);

  bool hasChangedConnectivity = false;

  if(triangulation->isEmpty()) {
    Modified();
    hasChangedConnectivity = true;
  }

  // Allocate the memory for the output scalar field
  vtkNew<vtkIntArray> outputMandatoryMinimum{};
  outputMandatoryMinimum->SetNumberOfTuples(input->GetNumberOfPoints());
  outputMandatoryMinimum->SetName("MinimumComponents");

  vtkNew<vtkIntArray> outputMandatoryJoinSaddle{};
  outputMandatoryJoinSaddle->SetNumberOfTuples(input->GetNumberOfPoints());
  outputMandatoryJoinSaddle->SetName("JoinSaddleComponents");

  vtkNew<vtkIntArray> outputMandatorySplitSaddle{};
  outputMandatorySplitSaddle->SetNumberOfTuples(input->GetNumberOfPoints());
  outputMandatorySplitSaddle->SetName("SplitSaddleComponents");

  vtkNew<vtkIntArray> outputMandatoryMaximum{};
  outputMandatoryMaximum->SetNumberOfTuples(input->GetNumberOfPoints());
  outputMandatoryMaximum->SetName("MaximumComponents");

  // Add array in the output data set
  outputMinimum->GetPointData()->AddArray(outputMandatoryMinimum);
  outputJoinSaddle->GetPointData()->AddArray(outputMandatoryJoinSaddle);
  outputSplitSaddle->GetPointData()->AddArray(outputMandatorySplitSaddle);
  outputMaximum->GetPointData()->AddArray(outputMandatoryMaximum);

  // Reset the baseCode object
  if((computeAll_) || (hasChangedConnectivity))
    this->flush();

  // Set the number of vertex
  this->setVertexNumber(input->GetNumberOfPoints());
  // Set the coordinates of each vertex
  std::array<double, 3> point{};
  for(int i = 0; i < input->GetNumberOfPoints(); i++) {
    input->GetPoint(i, point.data());
    this->setVertexPosition(i, point.data());
  }
  // Set the void pointers to the upper and lower bound fields
  this->setLowerBoundFieldPointer(inputLowerBoundField->GetVoidPointer(0));
  this->setUpperBoundFieldPointer(inputUpperBoundField->GetVoidPointer(0));
  // Set the output data pointers
  this->setOutputMinimumDataPointer(outputMandatoryMinimum->GetVoidPointer(0));
  this->setOutputJoinSaddleDataPointer(
    outputMandatoryJoinSaddle->GetVoidPointer(0));
  this->setOutputSplitSaddleDataPointer(
    outputMandatorySplitSaddle->GetVoidPointer(0));
  this->setOutputMaximumDataPointer(outputMandatoryMaximum->GetVoidPointer(0));
  // Set the triangulation object
  // Set the offsets
  this->setSoSoffsets();
  // Simplification threshold

  this->setSimplificationThreshold(simplificationThreshold_);

  // Execute process
  if(computeAll_) {
    // Calling the executing package
    switch(inputUpperBoundField->GetDataType()) {
      vtkTemplateMacro(this->execute<VTK_TT>(*triangulation));
    }
    computeAll_ = false;
    simplify_ = false;
    computeMinimumOutput_ = true;
    computeJoinSaddleOutput_ = true;
    computeSplitSaddleOutput_ = true;
    computeMaximumOutput_ = true;
  }

  // Simplification
  if(simplify_) {
    // Simplify
    this->simplifyJoinTree();
    this->buildJoinTreePlanarLayout();
    this->simplifySplitTree();
    this->buildSplitTreePlanarLayout();
    // Recompute the outputs
    computeMinimumOutput_ = true;
    computeJoinSaddleOutput_ = true;
    computeSplitSaddleOutput_ = true;
    computeMaximumOutput_ = true;
  }
  // Outputs
  if(computeMinimumOutput_) {
    if(outputAllMinimumComponents_)
      this->outputAllMinima();
    else
      this->outputMinimum(outputMinimumComponentId_);
    computeMinimumOutput_ = false;
  }
  if(computeJoinSaddleOutput_) {
    if(outputAllJoinSaddleComponents_)
      this->outputAllJoinSaddle(*triangulation);
    else
      this->outputJoinSaddle(outputJoinSaddleComponentId_, *triangulation);
    computeJoinSaddleOutput_ = false;
  }
  if(computeSplitSaddleOutput_) {
    if(outputAllSplitSaddleComponents_)
      this->outputAllSplitSaddle(*triangulation);
    else
      this->outputSplitSaddle(outputSplitSaddleComponentId_, *triangulation);
    computeSplitSaddleOutput_ = false;
  }
  if(computeMaximumOutput_) {
    if(outputAllMaximumComponents_)
      this->outputAllMaxima();
    else
      this->outputMaximum(outputMaximumComponentId_);
    computeMaximumOutput_ = false;
  }

  buildVtkTree(outputJoinTree, ttk::MandatoryCriticalPoints::TreeType::JoinTree,
               mdtJoinTree_, mdtJoinTreePointXCoord_, mdtJoinTreePointYCoord_,
               mdtJoinTreePointComponentId_, mdtJoinTreePointType_,
               mdtJoinTreePointLowInterval_, mdtJoinTreePointUpInterval_,
               mdtJoinTreeEdgeSwitchable_);
  buildVtkTree(outputSplitTree,
               ttk::MandatoryCriticalPoints::TreeType::SplitTree, mdtSplitTree_,
               mdtSplitTreePointXCoord_, mdtSplitTreePointYCoord_,
               mdtSplitTreePointComponentId_, mdtSplitTreePointType_,
               mdtSplitTreePointLowInterval_, mdtSplitTreePointUpInterval_,
               mdtSplitTreeEdgeSwitchable_);

  return 1;
}
