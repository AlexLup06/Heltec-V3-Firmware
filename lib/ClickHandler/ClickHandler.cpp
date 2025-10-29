#include "ClickHandler.h"

ClickHandler::ClickHandler(int pin, bool activeLow)
    : _pin(pin), _activeLow(activeLow) {}

void ClickHandler::begin()
{
    pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT);
    _lastState = _activeLow ? HIGH : LOW;
}

void ClickHandler::onSingleClick(Callback cb) { _singleClickCb = cb; }
void ClickHandler::onDoubleClick(Callback cb) { _doubleClickCb = cb; }
void ClickHandler::onTripleClick(Callback cb) { _tripleClickCb = cb; }
void ClickHandler::onQuadrupleClick(Callback cb) { _quadrupleClick = cb; }

void ClickHandler::trigger(int count)
{
    DEBUG_PRINTF("Clicked %d times!", count);
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
    default:
        break;
    }
}

void ClickHandler::update()
{
    bool reading = digitalRead(_pin);
    if (_activeLow)
        reading = !reading; // normalize: pressed = true

    if (reading != _lastState)
    {
        _lastDebounceTime = millis();
    }

    if ((millis() - _lastDebounceTime) > _debounceMs)
    {
        if (reading && !_buttonPressed)
        {
            _buttonPressed = true;
            _clickCount++;
            _lastClickTime = millis();
        }
        else if (!reading && _buttonPressed)
        {
            _buttonPressed = false;
        }

        if ((millis() - _lastClickTime) > _multiClickTimeout && _clickCount > 0)
        {
            trigger(_clickCount);
            _clickCount = 0;
        }
    }

    _lastState = reading;
}
