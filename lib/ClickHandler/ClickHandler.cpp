#include "ClickHandler.h"

ClickHandler::ClickHandler(int pin, bool activeLow)
    : _pin(pin), _activeLow(activeLow), inputActive(false) {}

void ClickHandler::begin(SX1262Public *radio_)
{
    pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT);
    _lastState = _activeLow ? HIGH : LOW;
    radio = radio_;
}

void ClickHandler::onSingleLongClick(Callback cb) { _singleLongClickCb = cb; }
void ClickHandler::onSingleClick(Callback cb) { _singleClickCb = cb; }
void ClickHandler::onDoubleClick(Callback cb) { _doubleClickCb = cb; }
void ClickHandler::onTripleClick(Callback cb) { _tripleClickCb = cb; }
void ClickHandler::onQuadrupleClick(Callback cb) { _quadrupleClick = cb; }

void ClickHandler::trigger(int count)
{
    Serial.printf("Clicked %d times!\n", count);
    switch (count)
    {
    case 1:
        if (_singleClickCb)
            _singleClickCb();
        break;
    case 2:
        if (_doubleClickCb)
            _doubleClickCb();
        break;
    case 3:
        if (_tripleClickCb)
            _tripleClickCb();
        break;
    case 4:
        if (_quadrupleClick)
            _quadrupleClick();
        break;
    default:
        break;
    }
    inputActive = false;
}

void ClickHandler::update()
{
    bool reading = digitalRead(_pin);
    if (_activeLow)
        reading = !reading;

    if (reading != _lastState)
    {
        _lastDebounceTime = millis();
    }

    if ((millis() - _lastDebounceTime) > _debounceMs)
    {
        if (reading && !_buttonPressed)
        {
            _buttonPressed = true;
            _pressStartTime = millis();
            _longClickTriggered = false;
            _clickCount++;
            _lastClickTime = millis();

            inputActive = true;
        }
        else if (!reading && _buttonPressed)
        {
            _buttonPressed = false;

            if (!_longClickTriggered)
            {
                if ((millis() - _lastClickTime) > _multiClickTimeout && _clickCount > 0)
                {
                    trigger(_clickCount);
                    _clickCount = 0;
                }
            }
            else
            {
                _clickCount = 0;
            }
        }

        if (_buttonPressed && !_longClickTriggered)
        {
            if ((millis() - _pressStartTime) >= 2000)
            {
                _longClickTriggered = true;

                if (_singleLongClickCb)
                    _singleLongClickCb();

                inputActive = false;
            }
        }

        if (!_buttonPressed && (millis() - _lastClickTime) > _multiClickTimeout && _clickCount > 0)
        {
            trigger(_clickCount);
            _clickCount = 0;
        }
    }

    _lastState = reading;
}
