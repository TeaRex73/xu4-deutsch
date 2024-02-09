/*
 * $Id$
 */

#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include <algorithm>
#include <vector>

#include "observer.h"


/**
 * Classes can report updates to a list of decoupled Observers by
 * extending this class.
 *
 * The O class parameter should be a pointer to the class of the
 * observable itself, so it can be passed in a typesafe manner to the
 * observers update method.
 *
 * The A class can be any additional information to pass to observers.
 * Observables that don't need to pass an argument when they update
 * observers should use the default "NoArg" class for the second
 * template parameter and pass nullptr to notifyObservers.
 */
template<class O, class A = NoArg *> class Observable {
public:
    Observable():
        changed(false), observers()
    {
    }

    virtual ~Observable() = default;

    void addObserver(Observer<O, A> *o)
    {
        typename std::vector<Observer<O, A> *>::const_iterator i =
            std::find(observers.cbegin(), observers.cend(), o);
        if (i == observers.cend()) {
            observers.push_back(o);
        }
    }

    int countObservers() const
    {
        return observers.size();
    }

    void deleteObserver(Observer<O, A> *o)
    {
        typename std::vector<Observer<O, A> *>::const_iterator i =
            std::find(observers.cbegin(), observers.cend(), o);
        if (i != observers.cend()) {
            observers.erase(i);
        }
    }

    void deleteObservers()
    {
        observers.clear();
    }

    bool hasChanged() const
    {
        return changed;
    }

    void notifyObservers(const A &arg)
    {
        if (!changed) {
            return;
        }
        // vector iterators are invalidated if erase
        // is called, so a copy is used to prevent
        // problems if the observer removes itself (or
        // otherwise changes the observer list)
        std::vector<Observer<O, A> *> tmp = observers;
        clearChanged();
        std::for_each(
            tmp.begin(),
            tmp.end(),
            [&](Observer<O, A> *v) -> void {
                v->update(static_cast<O>(this), arg);
            }
        );
    }

protected:
    void clearChanged()
    {
        changed = false;
    }

    void setChanged()
    {
        changed = true;
    }

private:
    bool changed;
    std::vector<Observer<O, A> *> observers;
};

#endif /* OBSERVABLE_H */
