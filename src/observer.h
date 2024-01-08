/*
 * $Id$
 */

#ifndef OBSERVER_H
#define OBSERVER_H

template<class O, class A> class Observable;

class NoArg;


/**
 * This is the interface a class must implement to watch an
 * Observable.
 */
template<class O, class A = NoArg *> class Observer {
public:
    virtual void update(O observable, A arg) = 0;

    virtual ~Observer()
    {
    }
};


/**
 * A specialized observer for watching observables that don't use the
 * "arg" parameter to update.
 */
template<class O> class Observer<O, NoArg *> {
public:
// Allow inheriting this multiple times with different O's
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
    virtual void update(O observable, NoArg *)
    {
        update(observable);
    }
#pragma GCC diagnostic pop

    virtual void update(O observable) = 0;

    virtual ~Observer()
    {
    }
};


/**
 * Just an empty marker class to identify observers that take no args
 * on update.
 */
class NoArg {
};

#endif /* OBSERVER_H */
