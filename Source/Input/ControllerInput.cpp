#include <Urho3D/Urho3DAll.h>
#include "ControllerInput.h"
#include "../MyEvents.h"
#include "../Global.h"
#include "Controllers/KeyboardInput.h"
#include "Controllers/MouseInput.h"
#include "Controllers/JoystickInput.h"

/// Construct.
ControllerInput::ControllerInput(Context* context) :
    Object(context),
	_multipleControllerSupport(true)
{

	context_->RegisterFactory<BaseInput>();
	context_->RegisterFactory<KeyboardInput>();
	context_->RegisterFactory<MouseInput>();
	context_->RegisterFactory<JoystickInput>();

	_inputHandlers[ControllerType::KEYBOARD] = context_->CreateObject<KeyboardInput>();
	_inputHandlers[ControllerType::MOUSE] = context_->CreateObject<MouseInput>();
	_inputHandlers[ControllerType::JOYSTICK] = context_->CreateObject<JoystickInput>();

	_controlMapNames[CTRL_FORWARD] = "Move forward";
	_controlMapNames[CTRL_BACK] = "Move backward";
	_controlMapNames[CTRL_LEFT] = "Strafe left";
	_controlMapNames[CTRL_RIGHT] = "Strafe right";
	_controlMapNames[CTRL_JUMP] = "Jump";
	_controlMapNames[CTRL_ACTION] = "Primary action";
	_controlMapNames[CTRL_SPRINT] = "Sprint";
	_controlMapNames[CTRL_UP] = "Move up";

	_configManager = new ConfigManager(context);

	_configurationFile = GetSubsystem<FileSystem>()->GetProgramDir() + "/Data/Config/controls.cfg";
	Init();
}

ControllerInput::~ControllerInput()
{
}

void ControllerInput::Init()
{
    // Subscribe to global events for camera movement
    SubscribeToEvents();

	LoadConfig();
}

void ControllerInput::LoadConfig()
{
	auto* cache = GetSubsystem<ResourceCache>();
	bool loaded = _configManager->Load(_configurationFile, true);
	if (!loaded) {
		URHO3D_LOGERROR("Unable to load configuration file '" + _configurationFile + "'");
		return;
	}

	for (auto it = _controlMapNames.Begin(); it != _controlMapNames.End(); ++it) {
		String controlName = (*it).second_;
		controlName.Replace(" ", "_");
		int controlCode = (*it).first_;
		if (_configManager->GetInt("keyboard", controlName, -1) != -1) {
			int key = _configManager->GetInt("keyboard", controlName, 0);
			_inputHandlers[ControllerType::KEYBOARD]->SetKeyToAction(key, controlCode);
		}
		if (_configManager->GetInt("mouse", controlName, -1) != -1) {
			int key = _configManager->GetInt("mouse", controlName, 0);
			_inputHandlers[ControllerType::MOUSE]->SetKeyToAction(key, controlCode);
		}
		if (_configManager->GetInt("joystick", controlName, -1) != -1) {
			int key = _configManager->GetInt("joystick", controlName, 0);
			_inputHandlers[ControllerType::JOYSTICK]->SetKeyToAction(key, controlCode);
		}
	}
}

void ControllerInput::SaveConfig()
{
	for (auto it = _controlMapNames.Begin(); it != _controlMapNames.End(); ++it) {
		String controlName = (*it).second_;
		controlName.Replace(" ", "_");
		int controlCode = (*it).first_;
		_configManager->Set("keyboard", controlName, "-1");
		_configManager->Set("mouse", controlName, "-1");
		_configManager->Set("joystick", controlName, "-1");
	}

	for (auto it = _inputHandlers.Begin(); it != _inputHandlers.End(); ++it) {
		HashMap<int, int> configMap = (*it).second_->GetConfigMap();
		int type = (*it).first_;
		String typeName;
		HashMap<int, String> map;
		map[ControllerType::KEYBOARD] = "keyboard";
		map[ControllerType::MOUSE] = "mouse";
		map[ControllerType::JOYSTICK] = "joystick";

		for (auto it2 = configMap.Begin(); it2 != configMap.End(); ++it2) {
			int controlCode = (*it2).first_;
		 	int keyCode = (*it2).second_;
			 if (_controlMapNames.Contains(controlCode) && !_controlMapNames[controlCode].Empty()) {
				String controlName = _controlMapNames[controlCode];
				controlName.Replace(" ", "_");
				String value = String(keyCode);
				_configManager->Set(map[type], controlName, value);
			}
		}
	}

	_configManager->Save(_configurationFile, true);
}

void ControllerInput::SubscribeToEvents()
{
	SubscribeToEvent(MyEvents::E_START_INPUT_MAPPING, URHO3D_HANDLER(ControllerInput, HandleStartInputListening));

	SubscribeToEvent("StartInputMappingConsole", URHO3D_HANDLER(ControllerInput, HandleStartInputListeningConsole));
	RegisterConsoleCommands();
}

void ControllerInput::ReleaseConfiguredKey(int key, int action)
{
	// Clear all input handler mappings against key and actions
	for (auto it = _inputHandlers.Begin(); it != _inputHandlers.End(); ++it) {
		(*it).second_->ReleaseAction(action);
		(*it).second_->ReleaseKey(key);
	}
}

void ControllerInput::SetConfiguredKey(int action, int key, String controller)
{
	// Clear previously assigned key and/or action
	ReleaseConfiguredKey(key, action);
	auto* input = GetSubsystem<Input>();
	if (controller == "keyboard") {
		_inputHandlers[ControllerType::KEYBOARD]->SetKeyToAction(key, action);
	}
	if (controller == "mouse") {
		_inputHandlers[ControllerType::MOUSE]->SetKeyToAction(key, action);
	}
	if (controller == "joystick") {
		_inputHandlers[ControllerType::JOYSTICK]->SetKeyToAction(key, action);
	}

	// Stop listening for keyboard key mapping
	for (auto it = _inputHandlers.Begin(); it != _inputHandlers.End(); ++it) {
		(*it).second_->StopMappingAction();
	}

	// Send out event with all the details about the mapped control
	using namespace MyEvents::InputMappingFinished;
	VariantMap data = GetEventDataMap();
	data[P_CONTROLLER] = controller;
	data[P_CONTROL_ACTION] = action;
	data[P_ACTION_NAME] = _controlMapNames[action];
	data[P_KEY] = key;
	data[P_KEY_NAME] = input->GetKeyName(key);
	SendEvent(MyEvents::E_INPUT_MAPPING_FINISHED, data);

	SaveConfig();
}

void ControllerInput::HandleStartInputListening(StringHash eventType, VariantMap& eventData)
{
	int activeAction = 0;

	using namespace MyEvents::StartInputMapping;
	if (eventData[P_CONTROL_ACTION].GetType() == VAR_INT) {
		activeAction = eventData[P_CONTROL_ACTION].GetInt();
	}
	if (eventData[P_CONTROL_ACTION].GetType() == VAR_STRING) {
		String control = eventData[P_CONTROL_ACTION].GetString();
		for (auto it = _controlMapNames.Begin(); it != _controlMapNames.End(); ++it) {
			if ((*it).second_ == control) {
				activeAction = (*it).first_;
			}
		}
	}

	if (activeAction > 0) {
		// Prepare all input handlers for key mapping against specific action
		for (auto it = _inputHandlers.Begin(); it != _inputHandlers.End(); ++it) {
			(*it).second_->StartMappingAction(activeAction);
		}
		URHO3D_LOGINFO("Starting to map action!");
	}
}

void ControllerInput::RegisterConsoleCommands()
{
	VariantMap data = GetEventDataMap();
    data["ConsoleCommandName"] = "map_input";
    data["ConsoleCommandEvent"] = "StartInputMappingConsole";
    data["ConsoleCommandDescription"] = "Listening for keystroke";
    SendEvent("ConsoleCommandAdd", data);
}

void ControllerInput::HandleStartInputListeningConsole(StringHash eventType, VariantMap& eventData)
{
	StringVector parameters = eventData["Parameters"].GetStringVector();
	if (parameters.Size() == 2) {
		using namespace MyEvents::StartInputMapping;
		VariantMap data = GetEventDataMap();
		data[P_CONTROL_ACTION] = parameters[1];
		SendEvent(MyEvents::E_START_INPUT_MAPPING, data);
		return;
	}

	URHO3D_LOGERROR("Invalid number of parameters!");
}

Controls ControllerInput::GetControls(int index)
{
	if (!_multipleControllerSupport) {
		index = 0;
	}
	return _controls[index];
}

void ControllerInput::UpdateYaw(float yaw, int index)
{
	if (!_multipleControllerSupport) {
		index = 0;
	}
	const float MOUSE_SENSITIVITY = 0.1f;
	_controls[index].yaw_ += MOUSE_SENSITIVITY * yaw;
}

void ControllerInput::UpdatePitch(float pitch, int index)
{
	if (!_multipleControllerSupport) {
		index = 0;
	}
	const float MOUSE_SENSITIVITY = 0.1f;
	_controls[index].pitch_ += MOUSE_SENSITIVITY * pitch;
	_controls[index].pitch_ = Clamp(_controls[index].pitch_, -90.0f, 90.0f);
}

void ControllerInput::CreateController(int controllerIndex)
{
	if (!_multipleControllerSupport) {
		return;
	}
	using namespace MyEvents::ControllerAdded;
	_controls[controllerIndex] = Controls();
	VariantMap data = GetEventDataMap();
	data[P_INDEX] = controllerIndex;
	SendEvent(MyEvents::E_CONTROLLER_ADDED, data);
}

void ControllerInput::DestroyController(int controllerIndex)
{
	if (!_multipleControllerSupport) {
		return;
	}
	// Don't allow destroying first input controller
	if (controllerIndex > 0) {
		_controls.Erase(controllerIndex);

		using namespace MyEvents::ControllerRemoved;

		VariantMap data = GetEventDataMap();
		data[P_INDEX] = controllerIndex;
		SendEvent(MyEvents::E_CONTROLLER_REMOVED, data);
	}
}

HashMap<int, String> ControllerInput::GetControlNames()
{
	return _controlMapNames;
}

String ControllerInput::GetActionKeyName(int action)
{
	for (auto it = _inputHandlers.Begin(); it != _inputHandlers.End(); ++it) {
		if ((*it).second_->IsActionUsed(action)) {
			String keyName = (*it).second_->GetActionKeyName(action);
			return keyName;
		}
	}

	return String::EMPTY;
}

void ControllerInput::SetActionState(int action, bool active, int index)
{
	if (!_multipleControllerSupport) {
		index = 0;
	}
	_controls[index].Set(action, active);
}

Vector<int> ControllerInput::GetControlIndexes()
{
	Vector<int> indexes;
	if (!_multipleControllerSupport) {
		indexes.Push(0);
		return indexes;
	}

	for (auto it = _controls.Begin(); it != _controls.End(); ++it) {
		indexes.Push((*it).first_);
	}
	return indexes;
}

void ControllerInput::SetMultipleControllerSupport(bool enabled)
{
	_multipleControllerSupport = enabled;
}