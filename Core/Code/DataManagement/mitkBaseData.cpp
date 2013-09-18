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


#include "mitkBaseData.h"

#include <mitkProportionalTimeGeometry.h>
#include <itkObjectFactoryBase.h>
#include <mitkException.h>


mitk::BaseData::BaseData() :
  m_RequestedRegionInitialized(false),
  m_SourceOutputIndexDuplicate(0), m_Initialized(true)
{
  m_TimeGeometry = mitk::ProportionalTimeGeometry::New();
  m_PropertyList = PropertyList::New();
}

mitk::BaseData::BaseData( const BaseData &other ):
itk::DataObject(), mitk::OperationActor(),
m_RequestedRegionInitialized(other.m_RequestedRegionInitialized),
m_SourceOutputIndexDuplicate(other.m_SourceOutputIndexDuplicate),
m_Initialized(other.m_Initialized)
{
  m_TimeGeometry = dynamic_cast<TimeGeometry *>(other.m_TimeGeometry->Clone().GetPointer());
  m_PropertyList = other.m_PropertyList->Clone();
}

mitk::BaseData::~BaseData()
{
}

void mitk::BaseData::InitializeTimeGeometry(unsigned int timeSteps)
{
  mitk::Geometry3D::Pointer g3d = mitk::Geometry3D::New();
  g3d->Initialize();

 if ( timeSteps > 1 )
 {
    mitk::ScalarType timeBounds[] = {0.0, 1.0};
    g3d->SetTimeBounds( timeBounds );
 }

  // The geometry is propagated automatically to the other items,
  // if EvenlyTimed is true...
  //Old timeGeometry->InitializeEvenlyTimed( g3d.GetPointer(), timeSteps );

  TimeGeometry::Pointer timeGeometry = this->GetTimeGeometry();
  timeGeometry->Expand(timeSteps);
  for (TimeStepType step = 0; step < timeSteps; ++step)
  {
    timeGeometry->SetTimeStepGeometry(g3d.GetPointer(),step);
  }
}

void mitk::BaseData::UpdateOutputInformation()
{
  if ( this->GetSource() )
  {
    this->GetSource()->UpdateOutputInformation();
  }
  if (m_TimeGeometry.IsNotNull())
  {
    m_TimeGeometry->UpdateBoundingBox();
  }
}

const mitk::TimeGeometry* mitk::BaseData::GetUpdatedTimeGeometry()
{
  SetRequestedRegionToLargestPossibleRegion();

  UpdateOutputInformation();

  return GetTimeGeometry();
}

void mitk::BaseData::Expand( unsigned int timeSteps )
{
  if (m_TimeGeometry.IsNotNull() )
  {
    ProportionalTimeGeometry * propTimeGeometry = dynamic_cast<ProportionalTimeGeometry*> (m_TimeGeometry.GetPointer());
    if (propTimeGeometry)
    {
      propTimeGeometry->Expand(timeSteps);
      return;
    }

    mitkThrow() << "TimeGeometry is of an unkown Type. Could not expand it. ";
  }
  else
  {
    this->InitializeTimeGeometry(timeSteps);
  }
}

const mitk::Geometry3D* mitk::BaseData::GetUpdatedGeometry(int t)
{
  SetRequestedRegionToLargestPossibleRegion();

  UpdateOutputInformation();

  return GetGeometry(t);
}

void mitk::BaseData::SetGeometry(Geometry3D* geometry)
{
  ProportionalTimeGeometry::Pointer timeGeometry = ProportionalTimeGeometry::New();
  if(geometry!=NULL)
  {
    timeGeometry->Initialize(geometry, 1);
  }
  SetTimeGeometry(timeGeometry);
  return;
}

void mitk::BaseData::SetTimeGeometry(TimeGeometry* geometry)
{
  m_TimeGeometry = geometry;
  this->Modified();
}

void mitk::BaseData::SetClonedGeometry(const Geometry3D* aGeometry3D)
{
  SetGeometry(static_cast<mitk::Geometry3D*>(aGeometry3D->Clone().GetPointer()));
}

void mitk::BaseData::SetClonedTimeGeometry(const TimeGeometry* geometry)
{
  itk::LightObject::Pointer clonedGeometry = geometry->Clone();
  SetTimeGeometry(dynamic_cast<TimeGeometry *>(clonedGeometry.GetPointer()));
}


void mitk::BaseData::SetClonedGeometry(const Geometry3D* aGeometry3D, unsigned int time)
{
  if (m_TimeGeometry)
  {
    m_TimeGeometry->SetTimeStepGeometry(static_cast<mitk::Geometry3D*>(aGeometry3D->Clone().GetPointer()),time);
  }
}

bool mitk::BaseData::IsEmptyTimeStep(unsigned int) const
{
  return IsInitialized() == false;
}

bool mitk::BaseData::IsEmpty() const
{
  if(IsInitialized() == false)
    return true;
  const TimeGeometry* timeGeometry = const_cast<BaseData*>(this)->GetUpdatedTimeGeometry();
  if(timeGeometry == NULL)
    return true;
  unsigned int timeSteps = timeGeometry->GetNumberOfTimeSteps();
  for ( unsigned int t = 0 ; t < timeSteps ; ++t )
  {
    if(IsEmptyTimeStep(t) == false)
      return false;
  }
  return true;
}

itk::SmartPointer<mitk::BaseDataSource> mitk::BaseData::GetSource() const
{
  return static_cast<mitk::BaseDataSource*>(Superclass::GetSource().GetPointer());
}

mitk::PropertyList::Pointer mitk::BaseData::GetPropertyList() const
{
  return m_PropertyList;
}


mitk::BaseProperty::Pointer mitk::BaseData::GetProperty(const char *propertyKey) const
{
  return m_PropertyList->GetProperty(propertyKey);
}

void mitk::BaseData::SetProperty(const char *propertyKey,
                                 BaseProperty* propertyValue)
{
  m_PropertyList->SetProperty(propertyKey, propertyValue);
}

void mitk::BaseData::SetPropertyList(PropertyList *pList)
{
  m_PropertyList = pList;
}

void mitk::BaseData::SetOrigin(const mitk::Point3D& origin)
{
  TimeGeometry* timeGeom = GetTimeGeometry();

  assert (timeGeom != NULL);
  Geometry3D* geometry;

  TimeStepType steps = timeGeom->GetNumberOfTimeSteps();
  for (TimeStepType timestep = 0; timestep < steps; ++timestep)
  {
    geometry = GetGeometry(timestep);
    if (geometry != NULL)
    {
      geometry->SetOrigin(origin);
    }
  }
}

unsigned long mitk::BaseData::GetMTime() const
{
  unsigned long time = Superclass::GetMTime();
  if(m_TimeGeometry.IsNotNull())
  {
    if((time < m_TimeGeometry->GetMTime()))
    {
      Modified();
      return Superclass::GetMTime();
    }
  }
  return time;
}

void mitk::BaseData::Graft(const itk::DataObject*)
{
  itkExceptionMacro(<< "Graft not implemented for mitk::BaseData subclass " << this->GetNameOfClass())
}

void mitk::BaseData::CopyInformation( const itk::DataObject* data )
{
  const Self* bd = dynamic_cast<const Self*>(data);
  if (bd != NULL)
  {
    m_PropertyList = bd->GetPropertyList()->Clone();
    if (bd->GetTimeGeometry()!=NULL)
    {
      itk::LightObject::Pointer clon = bd->GetTimeGeometry()->Clone();
      m_TimeGeometry = dynamic_cast<TimeGeometry *> (clon.GetPointer());
    }
  }
  else
  {
    // pointer could not be cast back down; this can be the case if your filters input
    // and output objects differ in type; then you have to write your own GenerateOutputInformation method
    itkExceptionMacro(<< "mitk::BaseData::CopyInformation() cannot cast "
      << typeid(data).name() << " to "
      << typeid(Self*).name() );
  }
}

bool mitk::BaseData::IsInitialized() const
{
  return m_Initialized;
}

void mitk::BaseData::Clear()
{
  this->ClearData();
  this->InitializeEmpty();
}

void mitk::BaseData::ClearData()
{
  if(m_Initialized)
  {
    ReleaseData();
    m_Initialized = false;
  }
}

void mitk::BaseData::ExecuteOperation(mitk::Operation* /*operation*/)
{
  //empty by default. override if needed!
}

void mitk::BaseData::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  os << std::endl;
  os << indent << " TimeGeometry: ";
  if(GetTimeGeometry() == NULL)
    os << "NULL" << std::endl;
  else
    GetTimeGeometry()->Print(os, indent);
}