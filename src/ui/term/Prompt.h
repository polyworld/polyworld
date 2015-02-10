#pragma once

#include <queue>

#include <QThread>

#include "Mutex.h"

class Prompt
{
 public:
	

	Prompt();
	virtual ~Prompt();

	bool isActive();
	char *getUserInput();

 private:
	class InputThread : public QThread
	{
	public:
		InputThread( Prompt *prompt );
		virtual ~InputThread();

		virtual void run();

	private:
		Prompt *prompt;
	};

	friend class InputThread;
	void addUserInput( char *input );
	void dismissed();

	InputThread *inputThread;
	typedef	std::queue<char *> StringQueue;
	StringQueue userInputQueue;
	bool pendingDismiss;
	SpinMutex lock;
};
