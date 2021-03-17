#include "subject.hpp"


void Subject::notify(Event event)
{
	// to avoid modifying observer list while iterating over it
	std::vector<Observer*> toDelete;

	for (Observer* observer : observers)
	{
		observer->onNotify(event);

		if (event.type == Event::MENU_CLOSE_ALL)
		{
			toDelete.push_back(observer);
		}
	}

	for (Observer* o : toDelete)
	{
		removeObserver(o);
	}
}

void Subject::addObserver(Observer* observer)
{
	observers.push_back(observer);
}

void Subject::removeObserver(Observer* observer)
{
	assert(std::find(observers.begin(), observers.end(), observer) != observers.end());
	observers.remove(observer);
}

void Subject::clearObservers()
{
	observers.clear();
}
