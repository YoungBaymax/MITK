/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/
#include <mitkContourModelMapper3D.h>

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkProperty.h>

mitk::ContourModelMapper3D::ContourModelMapper3D()
{
}


mitk::ContourModelMapper3D::~ContourModelMapper3D()
{
}


const mitk::ContourModel* mitk::ContourModelMapper3D::GetInput( void )
{
  //convient way to get the data from the dataNode
  return static_cast< const mitk::ContourModel * >( this->GetData() );
}


vtkProp* mitk::ContourModelMapper3D::GetVtkProp(mitk::BaseRenderer* renderer)
{  
  //return the actor corresponding to the renderer
  return m_LSH.GetLocalStorage(renderer)->m_Actor;
}


void mitk::ContourModelMapper3D::MitkRenderOverlay(BaseRenderer* renderer)
{
  if ( this->IsVisible(renderer)==false )
    return;
  if ( this->GetVtkProp(renderer)->GetVisibility() )
  {
    this->GetVtkProp(renderer)->RenderOverlay(renderer->GetVtkRenderer());
  }
}



void mitk::ContourModelMapper3D::MitkRenderOpaqueGeometry(BaseRenderer* renderer)
{
  if ( this->IsVisible( renderer )==false )
    return;
  if ( this->GetVtkProp(renderer)->GetVisibility() )
  {
    this->GetVtkProp(renderer)->RenderOpaqueGeometry( renderer->GetVtkRenderer() );
  }
}



void mitk::ContourModelMapper3D::MitkRenderTranslucentGeometry(BaseRenderer* renderer)
{
  if ( this->IsVisible(renderer)==false )
    return;
  if ( this->GetVtkProp(renderer)->GetVisibility() )
  {
    this->GetVtkProp(renderer)->RenderTranslucentPolygonalGeometry(renderer->GetVtkRenderer());
  }
}



void mitk::ContourModelMapper3D::MitkRenderVolumetricGeometry(BaseRenderer* renderer)
{
  if(IsVisible(renderer)==false)
    return;
  if ( GetVtkProp(renderer)->GetVisibility() )
  {
    this->GetVtkProp(renderer)->RenderVolumetricGeometry(renderer->GetVtkRenderer());
  }
}



void mitk::ContourModelMapper3D::GenerateDataForRenderer( mitk::BaseRenderer *renderer )
{
  /* First convert the contourModel to vtkPolyData, then tube filter it and
   * set it input for our mapper
   */

  LocalStorage *localStorage = m_LSH.GetLocalStorage(renderer);

  mitk::ContourModel* inputContour  = static_cast< mitk::ContourModel* >( this->GetData() );

  localStorage->m_OutlinePolyData = this->CreateVtkPolyDataFromContour(inputContour);

  this->ApplyContourProperties(renderer);

  //tube filter the polyData
  localStorage->m_TubeFilter->SetInput(localStorage->m_OutlinePolyData);
  localStorage->m_TubeFilter->SetRadius(0.2);
  localStorage->m_TubeFilter->Update();
  localStorage->m_Mapper->SetInput(localStorage->m_TubeFilter->GetOutput());

}



void mitk::ContourModelMapper3D::Update(mitk::BaseRenderer* renderer)
{
  if ( !this->IsVisible( renderer ) )
  {
    return;
  }

  mitk::ContourModel* data  = static_cast< mitk::ContourModel*>( this->GetData() );
  if ( data == NULL )
  {
    return;
  }

  // Calculate time step of the input data for the specified renderer (integer value)
  this->CalculateTimeStep( renderer );

  LocalStorage *localStorage = m_LSH.GetLocalStorage(renderer);
  // Check if time step is valid
  const TimeSlicedGeometry *dataTimeGeometry = data->GetTimeSlicedGeometry();
  if ( ( dataTimeGeometry == NULL )
    || ( dataTimeGeometry->GetTimeSteps() == 0 )
    || ( !dataTimeGeometry->IsValidTime( renderer->GetTimeStep() ) ) )
  {
    //clear the rendered polydata
    localStorage->m_Mapper->SetInput(vtkSmartPointer<vtkPolyData>::New());
    return;
  }

  const DataNode *node = this->GetDataNode();
  data->UpdateOutputInformation();

  //check if something important has changed and we need to rerender
  if ( (localStorage->m_LastUpdateTime < node->GetMTime()) //was the node modified?
    || (localStorage->m_LastUpdateTime < data->GetPipelineMTime()) //Was the data modified?
    || (localStorage->m_LastUpdateTime < renderer->GetCurrentWorldGeometry2DUpdateTime()) //was the geometry modified?
    || (localStorage->m_LastUpdateTime < renderer->GetCurrentWorldGeometry2D()->GetMTime())
    || (localStorage->m_LastUpdateTime < node->GetPropertyList()->GetMTime()) //was a property modified?
    || (localStorage->m_LastUpdateTime < node->GetPropertyList(renderer)->GetMTime()) )
  {
    this->GenerateDataForRenderer( renderer );
  }

  // since we have checked that nothing important has changed, we can set
  // m_LastUpdateTime to the current time
  localStorage->m_LastUpdateTime.Modified();
}



vtkSmartPointer<vtkPolyData> mitk::ContourModelMapper3D::CreateVtkPolyDataFromContour(mitk::ContourModel* inputContour)
{
  unsigned int timestep = this->GetTimestep();

  //the points to draw
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  //the lines to connect the points
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  // Create a polydata to store everything in
  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();

  //iterate over the control points
  mitk::ContourModel::VertexIterator current = inputContour->IteratorBegin(timestep);
  mitk::ContourModel::VertexIterator next = inputContour->IteratorBegin(timestep);
  if(next != inputContour->IteratorEnd(timestep))
  {
    next++;

    mitk::ContourModel::VertexIterator end = inputContour->IteratorEnd(timestep);

    while(next != end)
    {
      mitk::ContourModel::VertexType* currentControlPoint = *current;
      mitk::ContourModel::VertexType* nextControlPoint = *next;

      vtkIdType p1 = points->InsertNextPoint(currentControlPoint->Coordinates[0], currentControlPoint->Coordinates[1], currentControlPoint->Coordinates[2]);
      vtkIdType p2 = points->InsertNextPoint(nextControlPoint->Coordinates[0], nextControlPoint->Coordinates[1], nextControlPoint->Coordinates[2]);
      //add the line between both contorlPoints
      lines->InsertNextCell(2);
      lines->InsertCellPoint(p1);
      lines->InsertCellPoint(p2);

      current++; 
      next++;
    }

    if(inputContour->IsClosed(timestep))
    {
      // If the contour is closed add a line from the last to the first control point
      mitk::ContourModel::VertexType* firstControlPoint = *(inputContour->IteratorBegin(timestep));
      mitk::ContourModel::VertexType* lastControlPoint = *(--(inputContour->IteratorEnd(timestep)));
      vtkIdType p2 = points->InsertNextPoint(lastControlPoint->Coordinates[0], lastControlPoint->Coordinates[1], lastControlPoint->Coordinates[2]);
      vtkIdType p1 = points->InsertNextPoint(firstControlPoint->Coordinates[0], firstControlPoint->Coordinates[1], firstControlPoint->Coordinates[2]);

      //add the line to the cellArray
      lines->InsertNextCell(2);
      lines->InsertCellPoint(p1);
      lines->InsertCellPoint(p2);
    }


    // Add the points to the dataset
    polyData->SetPoints(points);
    // Add the lines to the dataset
    polyData->SetLines(lines);
  }
  return polyData;
}



void mitk::ContourModelMapper3D::ApplyContourProperties(mitk::BaseRenderer* renderer)
{
  LocalStorage *localStorage = m_LSH.GetLocalStorage(renderer);

  float binaryOutlineWidth(1.0);
  if (this->GetDataNode()->GetFloatProperty( "contour width", binaryOutlineWidth, renderer ))
  {
    localStorage->m_Actor->GetProperty()->SetLineWidth(binaryOutlineWidth);
  }

  localStorage->m_Actor->GetProperty()->SetColor(0.9, 1.0, 0.1);
}


/*+++++++++++++++++++ LocalStorage part +++++++++++++++++++++++++*/

mitk::ContourModelMapper3D::LocalStorage* mitk::ContourModelMapper3D::GetLocalStorage(mitk::BaseRenderer* renderer)
{
  return m_LSH.GetLocalStorage(renderer);
}


mitk::ContourModelMapper3D::LocalStorage::LocalStorage()
{
  m_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  m_Actor = vtkSmartPointer<vtkActor>::New();
  m_OutlinePolyData = vtkSmartPointer<vtkPolyData>::New();
  m_TubeFilter = vtkSmartPointer<vtkTubeFilter>::New();

  //set the mapper for the actor
  m_Actor->SetMapper(m_Mapper);
}


void mitk::ContourModelMapper3D::SetDefaultProperties(mitk::DataNode* node, mitk::BaseRenderer* renderer, bool overwrite)
{
  node->AddProperty( "color", ColorProperty::New(1.0,0.0,0.0), renderer, overwrite );
  node->AddProperty( "contour width", mitk::FloatProperty::New( 1.0 ), renderer, overwrite );

  Superclass::SetDefaultProperties(node, renderer, overwrite);
}