/// \class ttkRipsPersistenceDiagram
/// \ingroup vtk
/// \author Mattéo Clémot <matteo.clemot@univ-lyon1.fr>
/// \date January 2024.
///
/// \brief TTK VTK-filter that wraps the ttk::RipsPersistenceDiagram module.
///
/// VTK wrapping code for the ttk::RipsPersistenceDiagram package.
///
/// \param Input Input table (vtkTable)
/// \param Output PersistenceDiagram (vtkUnstructuredGrid)
///
/// This filter can be used as any other VTK filter (for instance, by using the
/// sequence of calls SetInputData(), Update(), GetOutputDataObject()).
///
/// See the related ParaView example state files for usage examples within a
/// VTK pipeline.
///
/// \sa ttk::RipsPersistenceDiagram
/// \sa ttkAlgorithm

#pragma once

// VTK Module
#include <ttkRipsPersistenceDiagramModule.h>

// VTK Includes
#include <ttkAlgorithm.h>

// TTK Base Includes
#include <RipsPersistenceDiagram.h>
#include <vtkUnstructuredGrid.h>

class TTKRIPSPERSISTENCEDIAGRAM_EXPORT ttkRipsPersistenceDiagram : public ttkAlgorithm, // we inherit from the generic ttkAlgorithm class
                                                                   protected ttk::RipsPersistenceDiagram  { // and we inherit from the base class
private:
  int DiagramToVTU(vtkUnstructuredGrid *vtu, const std::vector<std::vector<pers_pair_t> > &diagram);

public:
  static ttkRipsPersistenceDiagram *New();
  vtkTypeMacro(ttkRipsPersistenceDiagram, ttkAlgorithm);

  vtkSetMacro(MaxDim, int);
  vtkGetMacro(MaxDim, int);

  vtkSetMacro(Threshold, double);
  vtkGetMacro(Threshold, double);

protected:
  ttkRipsPersistenceDiagram();
  ~ttkRipsPersistenceDiagram() override = default;

  int FillInputPortInformation(int port, vtkInformation *info) override;

  int FillOutputPortInformation(int port, vtkInformation *info) override;

  int RequestData(vtkInformation *request,
                  vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector) override;
};
