#pragma once
class Job
{
public:
	virtual void Execute(){}
};

class LambdaJob : public Job 
{
public:
	LambdaJob(function<void()> f):_func(move(f)){}
	void Execute()override { _func(); }

private:
	function<void()> _func;
};

class JobQueue
{
public:
	void Push(shared_ptr<Job> job)
	{
		WRITE_LOCK;
		_jobs.push(job);
	}

	shared_ptr<Job> Pop()
	{
		WRITE_LOCK;
		if (_jobs.empty())
			return nullptr;

		shared_ptr<Job> ret= _jobs.front();
		_jobs.pop();
		return ret;
	}

private:
	USE_LOCK;
	queue<shared_ptr<Job>> _jobs;
};