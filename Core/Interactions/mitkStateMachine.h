#ifndef STATEMACHINE_H_HEADER_INCLUDED_C18896BD
#define STATEMACHINE_H_HEADER_INCLUDED_C18896BD

#include "mitkCommon.h"
#include <itkObject.h>
#include "mitkOperationActor.h"
#include "mitkState.h"
#include "mitkStateEvent.h"
#include "mitkUndoModel.h"
#include "mitkUndoController.h"
#include <string>


namespace mitk {

//##ModelId=3E5A397B01D5
//##Documentation
//## @brief superior statemachine
//## @ingroup Interaction
//## realizes the methods, that every statemachine has to have.
//## Undo can be enabled and disabled through EnableUndo
//## Developers must derive its statemachines and implement ExecuteAction
class StateMachine : public itk::Object, public mitk::OperationActor
{
  public:
  mitkClassMacro(StateMachine,itk::Object);

  //##ModelId=3E5B2DB301FD
  //##Documentation
  //## Constructor
  //## @brief connects the current State to a StateMachine of Type type;
  //## 
  //## Because of the use of Smartpointers in DataTree and the use of observer-technologies, a StateMachine is a Subclass of itk::Object.
  //## default of m_UndoEnabled is true;
  StateMachine(const char * type);

  ~StateMachine(){}

  //##ModelId=3E5B2E660087
  std::string GetType() const;

  //##ModelId=3E5B2DE30378
  //##Documentation
  //## @brief handles an Event accordingly to its current State
  //##
  //## statechange with Undo functionality;
  //## groupEventId and objectEventId are use to combine Operations so that
  //## they can be undone together or seperately.
  //## EventMapper gives each event a new objectEventId
  //## and a StateMachine::ExecuteAction can descide weather it gets a
  //## new GroupEventId or not, depending on its state (e.g. finishedNewObject then new GroupEventId)
  virtual bool HandleEvent(StateEvent const* stateEvent, int objectEventId, int groupEventId);

  //##ModelId=3EDCAECB0175
  //##Documentation
  //## if set to true, then UndoFunctionality is enabled
  //## if false, then Undo is disabled
  void EnableUndo(bool enable);

  //##Documentation
  //## so that UndoModel can call ExecuteOperation for Undo!
  friend class UndoModel;

 protected:

  //##ModelId=3E5B2E170228
  //##Documentation
  //## @brief Method called in HandleEvent after Statechange.
  //##
  //## Each statechange has actions, which can be assigned by it's number.
  //## If you are developing a new statemachine, declare all your operations here and send them to Undo-Controller and to the Data.
  virtual bool ExecuteAction(Action* action, mitk::StateEvent const* stateEvent, int objectEventId, int groupEventId)= 0;

  //##Documentation
  //## @brief returns the current state
  const State* GetCurrentState() const;

  //##ModelId=3EDCAECB00B9
  //##Documentation
  //## @brief if true, then UndoFunctionality is enabled
  //## default on true;
  bool m_UndoEnabled;

  //Documentation
  //##ModelId=3EF099EA03C0
  //## @brief friend protected function of OperationEvent, that way all StateMachines can increment GroupEventId!
  int IncCurrGroupEventId();

  //##ModelId=3EDCAECB0128
  //##Documentation
  //## @brief holds an UndoController, that can be accessed from all StateMachines. For ExecuteAction
  UndoController* m_UndoController;

 protected:
  //##ModelId=3EAEEDC603D9
  //##Documentation
  //## @brief A statemachine is also an OperationActor due to the UndoMechanism. 
  //## 
  //## The statechange is done in ExecuteOperation, so that the statechange can be undone by UndoMechanism.
  //## Is set private here and in superclass it is set public, so UndoController
  //## can reach ist, but it can't be overwritten by a subclass
  virtual void ExecuteOperation(Operation* operation);

 private:

  //##ModelId=3E5B2D66027E
  std::string m_Type;

  //##ModelId=3E5B2D8F02B9
  //##Documentation
  //## @brief holds the current state the machine is in
  State* m_CurrentState;

 };

 } // namespace mitk



 #endif /* STATEMACHINE_H_HEADER_INCLUDED_C18896BD */

