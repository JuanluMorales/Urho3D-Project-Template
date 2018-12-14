#include <Urho3D/Urho3DAll.h>
#include "Message.h"
#include "../Audio/AudioManagerDefs.h"
#include "../MyEvents.h"

/// Construct.
Message::Message(Context* context) :
    Object(context)
{
    Init();
}

Message::~Message()
{
}

void Message::Init()
{
    SubscribeToEvents();
}

void Message::SubscribeToEvents()
{
    SubscribeToEvent("ShowAlertMessage", URHO3D_HANDLER(Message, HandleShowMessage));
}

void Message::HandleShowMessage(StringHash eventType, VariantMap& eventData)
{
    String title = eventData["Title"].GetString();
    String message = eventData["Message"].GetString();

    new MessageBox(context_, message, title);
}
