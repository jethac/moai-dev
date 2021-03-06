#include "SledgeInputManager.h"

float SledgeInputManager::deadzone_thumbLeft = 0.0f;
float SledgeInputManager::deadzone_thumbRight = 0.0f;
float SledgeInputManager::deadzone_trigger = 0.0f;

SledgeInputManager::SledgeInputManager()
{

}

SledgeInputManager::~SledgeInputManager()
{

}

void SledgeInputManager::doAKUInit()
{
	AKUSetInputConfigurationName("AKUSDL2");
	AKUReserveInputDevices(SledgeInputDevice::ID_TOTAL);

	doAKUDeviceInit(SledgeInputDevice::ID_DEVICE);

	// disable controller events; we'll manually poll, thanks
	// ...or not? jesus.
	SDL_GameControllerEventState(SDL_IGNORE);

	setDeadzones
		((float)LEFT_THUMB_DEADZONE, (float)RIGHT_THUMB_DEADZONE, (float)TRIGGER_THRESHOLD);

	num_joysticks = SDL_NumJoysticks();
	printf("joystick count: %d\n", num_joysticks);
	int controllerIdx = 1;
	for (int i = 0; i < num_joysticks; ++i)
	{
		printf("\t[controller %d]\n", i+1);
		if (SDL_IsGameController(i))
		{
			printf("\t\tGame controller?\tYES\n");
			SDL_GameController* gc = SDL_GameControllerOpen(i);
			controllers.push_back(gc);
			NormalizedController nc;// = new NormalizedController;
			
			for (int j = 0; j < SDL_CONTROLLER_BUTTON_MAX; ++j)
			{
				ButtonState_Old.state[j] = false;
			}
			/*
			for (int j = 0; j < SDL_CONTROLLER_BUTTON_MAX; ++j)
			{
				printf("[%d][%d]: %d\n", i, j, nc->lastButtonState.state[j]);
			}
			*/
			
			controllers_normalized.push_back(nc);
			doAKUDeviceInit((SledgeInputDevice::InputDevice_ID)(i+1));
		} else {
			printf("\t\tGame controller?\tNO\n");
		}

		char* controllerName =
			const_cast<char*>(SDL_GameControllerNameForIndex(i));
		

		if (controllerName) {
			printf(
				"\t\tname: '%s'\n",
				controllerName
			);
		} else {
			printf("\t\tname: NONE\n");
		}
		
	}
}

void SledgeInputManager::doAKUDeviceInit(
	SledgeInputDevice::InputDevice_ID p_id	
)
{
	AKUSetInputDevice(
		p_id,
		SledgeInputDevice::DeviceName[p_id]
		);

	SledgeInputDeviceType::InputDeviceType_ID
		devicetype = SledgeInputDevice::DeviceType[p_id];

	switch (devicetype)
	{
	case SledgeInputDeviceType::IDT_DEVICE:
		initDevice(p_id);
		break;

	// @todo	actually implement this
	case SledgeInputDeviceType::IDT_PAD:
		initPad(p_id);
		break;

	default:
		printf("Unhandled device type.");
		break;
	}
}

void SledgeInputManager::initPad(
	SledgeInputDevice::InputDevice_ID p_id
)
{
	AKUReserveInputDeviceSensors(
		p_id,
		SledgePadSensorAxes::PS_TOTAL
		);
	AKUSetInputDeviceJoystick(
		p_id,
		SledgePadSensorAxes::PS_STICK_LEFT,
		SledgePadSensorAxes::SensorName[SledgePadSensorAxes::PS_STICK_LEFT]
	);
	AKUSetInputDeviceJoystick(
		p_id,
		SledgePadSensorAxes::PS_STICK_RIGHT,
		SledgePadSensorAxes::SensorName[SledgePadSensorAxes::PS_STICK_RIGHT]
	);
	AKUSetInputDeviceJoystick(
		p_id,
		SledgePadSensorAxes::PS_TRIGGERS,
		SledgePadSensorAxes::SensorName[SledgePadSensorAxes::PS_TRIGGERS]
	);
	AKUSetInputDeviceKeyboard(
		p_id,
		SledgePadSensorAxes::PS_BUTTONS, 
		SledgePadSensorAxes::SensorName[SledgePadSensorAxes::PS_BUTTONS]
	);
}


void SledgeInputManager::initDevice(
	SledgeInputDevice::InputDevice_ID p_id
)
{
	AKUReserveInputDeviceSensors(
		p_id,
		SledgeDeviceSensor::IDS_TOTAL
	);
	AKUSetInputDeviceKeyboard(
		p_id,
		SledgeDeviceSensor::IDS_KEYBOARD,
		SledgeDeviceSensor::SensorName[SledgeDeviceSensor::IDS_KEYBOARD]
	);
	AKUSetInputDevicePointer(
		p_id,
		SledgeDeviceSensor::IDS_POINTER,
		SledgeDeviceSensor::SensorName[SledgeDeviceSensor::IDS_POINTER]
	);
	AKUSetInputDeviceButton(
		p_id,
		SledgeDeviceSensor::IDS_MOUSE_LEFT,
		SledgeDeviceSensor::SensorName[SledgeDeviceSensor::IDS_MOUSE_LEFT]
	);
	AKUSetInputDeviceButton(
		p_id,
		SledgeDeviceSensor::IDS_MOUSE_MIDDLE,
		SledgeDeviceSensor::SensorName[SledgeDeviceSensor::IDS_MOUSE_MIDDLE]
	);
	AKUSetInputDeviceButton(
		p_id,
		SledgeDeviceSensor::IDS_MOUSE_RIGHT,
		SledgeDeviceSensor::SensorName[SledgeDeviceSensor::IDS_MOUSE_RIGHT]
	);
}

void SledgeInputManager::doOnTick()
{
	SDL_Event event;
	int _count = 0;
	SDL_GameControllerEventState(SDL_QUERY);
	
	if(controllers_normalized.size() != 0)
	{
		controllers_normalized[0] = postprocessController(controllers[0]);
		pollPadButtons(controllers[0], SledgeInputDevice::ID_PAD_0);
		updateAKU_Controller(
			SledgeInputDevice::ID_PAD_0,
			&controllers_normalized[0]
		);
	}
}

void SledgeInputManager::setDeadzones
	(float p_thumbLeft, float p_thumbRight, float p_trigger)
{
	SledgeInputManager::deadzone_thumbLeft = p_thumbLeft;
	SledgeInputManager::deadzone_thumbRight = p_thumbRight;
	SledgeInputManager::deadzone_trigger = p_trigger;
}

void SledgeInputManager::inputNotify_onKeyDown(SDL_KeyboardEvent* p_event)
{
	bool bIsDown = p_event->state == SDL_PRESSED;
	SDL_Scancode scancode = p_event->keysym.scancode;
	SDL_Scancode scancode2 = SDL_GetScancodeFromKey(p_event->keysym.sym);

	if(scancode != scancode2)
		scancode = scancode2;

	AKUEnqueueKeyboardEvent(
		SledgeInputDevice::ID_DEVICE,
		SledgeDeviceSensor::IDS_KEYBOARD,
		scancode2,
		bIsDown
	);
}

void SledgeInputManager::inputNotify_onMouseMove(SDL_MouseMotionEvent* p_event)
{
	AKUEnqueuePointerEvent (
		SledgeInputDevice::ID_DEVICE,
		SledgeDeviceSensor::IDS_POINTER,
		p_event->x,
		p_event->y
	);
}
void SledgeInputManager::inputNotify_onMouseButton(SDL_MouseButtonEvent* p_event)
{
	// @ todo	Add keyboard modifier support.
	switch (p_event->button)		
	{
	case SDL_BUTTON_LEFT:
		AKUEnqueueButtonEvent(
			SledgeInputDevice::ID_DEVICE,
			SledgeDeviceSensor::IDS_MOUSE_LEFT,
			(p_event->state == SDL_PRESSED)
			);
		break;
	case SDL_BUTTON_MIDDLE:
		AKUEnqueueButtonEvent(
			SledgeInputDevice::ID_DEVICE,
			SledgeDeviceSensor::IDS_MOUSE_MIDDLE,
			(p_event->state == SDL_PRESSED)
			);
		break;
	case SDL_BUTTON_RIGHT:
		AKUEnqueueButtonEvent(
			SledgeInputDevice::ID_DEVICE,
			SledgeDeviceSensor::IDS_MOUSE_RIGHT,
			(p_event->state == SDL_PRESSED)
			);
		break;
	}
}

void SledgeInputManager::inputNotify_onPadAxisMove(SDL_ControllerAxisEvent* p_event)
{
	//printf("SDL_ControllerAxisEvent [controller: %d]\n", p_event->which);
	printf(
		"\t%s: %d\n",
		JethaSDLControllerAxis::AxisName[p_event->axis],
		p_event->value
	);
}

vec2f SledgeInputManager::postprocessThumbstick(
	SDL_GameController* p_controller,
	SDL_CONTROLLER_AXIS p_axisX,
	SDL_CONTROLLER_AXIS p_axisY,
	const int p_deadzone
)
{
	float X_raw =
		(float)SDL_GameControllerGetAxis(p_controller, p_axisX);
	float Y_raw =
		(float)SDL_GameControllerGetAxis(p_controller, p_axisY);
	float Deadzone = (float)p_deadzone;

	vec2f result;

	// use raw values to calculate normalized direction vector
	float Magnitude_raw =
		sqrt(X_raw * X_raw + Y_raw * Y_raw);
	float X_dir = X_raw / Magnitude_raw;
	float Y_dir = Y_raw / Magnitude_raw;
	
	// enforce deadzone to get normalized magnitude
	float Magnitude_norm = 0.0f;
	if(Magnitude_raw > Deadzone)
	{
		if(Magnitude_raw > 32767.0f) Magnitude_raw = 32767.0f;
		Magnitude_raw -= Deadzone;
		Magnitude_norm = Magnitude_raw / (32767.0f - Deadzone);
	} else {
		Magnitude_raw = 0.0f;
		Magnitude_norm = 0.0f;
	}

	// multiply normalized magnitude by direction vector to get 
	// final result
	result.x = Magnitude_norm * X_dir;
	result.y = Magnitude_norm * Y_dir;

	return result;
}

float SledgeInputManager::postprocessTrigger(
	SDL_GameController* p_controller,
	SDL_CONTROLLER_AXIS p_axisT,
	const int p_deadzone
	)
{
	float result = 0.0f;

	float T_raw =
		(float)SDL_GameControllerGetAxis(p_controller, p_axisT);
	float T_norm = 0.0f;
	float Deadzone = (float)p_deadzone;

	if(T_raw > Deadzone)
	{
		//result = T_raw;

		// clip magnitude
		if(T_raw > (float)AXIS_MAX) T_raw = (float)AXIS_MAX;

		T_norm = (T_raw - Deadzone) / ((float)AXIS_MAX - Deadzone);
		//printf("trigger magnitude: %0.2f\n", T_raw);

		result = T_norm;
	}


	return result;
}


NormalizedController SledgeInputManager::postprocessController(
	SDL_GameController* p_controller
)
{
	NormalizedController result;

	result.stick_left = postprocessThumbstick(
		p_controller,
		SDL_CONTROLLER_AXIS_LEFTX,
		SDL_CONTROLLER_AXIS_LEFTY,
		LEFT_THUMB_DEADZONE
	);
	result.stick_right = postprocessThumbstick(
		p_controller,
		SDL_CONTROLLER_AXIS_RIGHTX,
		SDL_CONTROLLER_AXIS_RIGHTY,
		RIGHT_THUMB_DEADZONE
	);
	result.triggers.x = postprocessTrigger(
		p_controller,
		SDL_CONTROLLER_AXIS_TRIGGERLEFT,
		TRIGGER_THRESHOLD
	);
	result.triggers.y = postprocessTrigger(
		p_controller,
		SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
		TRIGGER_THRESHOLD
	);


	return result;
}

void SledgeInputManager::pollPadButtons(
	SDL_GameController* p_controller,
	SledgeInputDevice::InputDevice_ID p_deviceid
)
{
	buttonState newState;
	int deviceIdx = p_deviceid - 1;

	//printf("pollPadButtons(p_deviceid: %d)\n", p_deviceid);

	for(int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
	{
		bool bDownThisFrame = SDL_GameControllerGetButton(p_controller, (SDL_CONTROLLER_BUTTON)i) == 1;

		newState.state[i] = bDownThisFrame;
		bool bDown =
			newState.state[i] == ButtonState_Old.state[i] ||
			(newState.state[i] == true && ButtonState_Old.state[i] == false); 

		if(ButtonState_Old.state[i] == true || newState.state[i] == true)
		{
			AKUEnqueueKeyboardEvent(
				p_deviceid,
				SledgePadSensorAxes::PS_BUTTONS,
				i,
				bDown
			);
			/*
			printf("[p_deviceid: %d][%d][new: %d][old: %d]<<down: %d>>\n",
				p_deviceid,
				i,
				newState.state[i],
				ButtonState_Old.state[i],
				//controllers_normalized[0].lastButtonState.state[i],
				bDown
				);
			*/
		}
	}
	//controllers_normalized[0].lastButtonState = newState;
	ButtonState_Old = newState;
	//controllers_normalized[0].lastButtonState = newState;
}

void SledgeInputManager::updateAKU_Controller(
	SledgeInputDevice::InputDevice_ID p_deviceid,
	NormalizedController* p_nc
)
{	
	//float scalingFactor = (float)AKU_SCALING_FACTOR;

	AKUEnqueueJoystickEvent (
		p_deviceid,
		SledgePadSensorAxes::PS_STICK_LEFT,
		p_nc->stick_left.x,
		p_nc->stick_left.y
		);
	AKUEnqueueJoystickEvent (
		p_deviceid,
		SledgePadSensorAxes::PS_STICK_RIGHT,
		p_nc->stick_right.x,
		p_nc->stick_right.y
		);
	AKUEnqueueJoystickEvent (
		p_deviceid,
		SledgePadSensorAxes::PS_TRIGGERS,
		p_nc->triggers.x,
		p_nc->triggers.y
		);
	
}