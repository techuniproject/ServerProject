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
	void Push(unique_ptr<Job> job)
	{
		WRITE_LOCK;
		_jobs.push(move(job));
	}

	unique_ptr<Job> Pop()
	{
		WRITE_LOCK;
		if (_jobs.empty())
			return nullptr;

		unique_ptr<Job> ret= move(_jobs.front());
		_jobs.pop();
		return ret;
	}

	void PopAll(queue<unique_ptr<Job>>& jobs){
		WRITE_LOCK;
		jobs.swap(_jobs);
	}

private:
	USE_LOCK;
	queue<unique_ptr<Job>> _jobs;
};
