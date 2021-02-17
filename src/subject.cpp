#include "subject.hpp"


void Subject::notify(const ECS::Entity& entity, Event event)
{
	for (Observer* observer: observers)
	{
		observer->onNotify(entity, event);
	}
}

void Subject::addObserver(Observer* observer) {
	observers.push_back(observer);
}

void Subject::removeObserver(Observer* observer) {
	//assert(std::find(observers.begin(), observers.end(), observer) != observers.end()))
	observers.remove(observer);
}