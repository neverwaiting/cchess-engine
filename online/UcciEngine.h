#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <sys/wait.h>

class UcciEngine
{
public:
	UcciEngine(const char* procName, const char* procPath, const char* moveCommand, const char* displayCommand = "")
		: moveCommand_(moveCommand), displayCommand_(displayCommand)
	{
		std::vector<int> a;
		forkRoutine(procName, procPath);
		send("ucci\n");
		send("setoption Threads 4\n");
		std::string s;
		do {
			read(s);
		} while(s.find("ucciok") == std::string::npos);
		std::cout << procName << " engine init success!" << std::endl;
	}

	~UcciEngine()
	{
		send("quit\n");
		wait(NULL);
	}

	void play(const char* iccs_mv)
	{
		std::string command = "position fen " + getFen() + " moves " + iccs_mv + "\n";
		send(command);
	}

	std::string search(const std::string& fen, int milliseconds)
	{
		std::string iccs_mv;
		std::string s = "position fen " + fen + "\n";
		send(s);
		char movestr[BUFSIZ] = {'\0'};
		snprintf(movestr, BUFSIZ, "%s %d\n", moveCommand_.c_str(), milliseconds);
		send(movestr);
		std::string str;
		std::string::size_type pos;
		do
		{
			read(str);
			pos = str.find("bestmove");
		} while (pos == std::string::npos);

		iccs_mv = str.substr(pos + 9, 4);
		std::cout << str << std::endl;
		return iccs_mv;
	}

	std::string getFen()
	{
		std::string fen;
		if (!displayCommand_.empty())
		{
			send(displayCommand_ + "\n");
			std::string s;
			std::string::size_type pos;
			do
			{
				read(s);
				pos = s.find("Fen: ");
			} while (pos == std::string::npos);
			auto begin = pos + 5;
			auto end = s.find("\n", begin);
			fen = s.substr(begin, end - begin);
		}
		return fen;
	}

private:
	void forkRoutine(const char* procName, const char* procPath)
	{
		int cfds[2];
		int sfds[2];
		assert(pipe(cfds) == 0 && pipe(sfds) == 0);
		pid_t pid = vfork();
		if (pid == 0)
		{
			close(cfds[1]);
			close(sfds[0]);
			dup2(cfds[0], STDIN_FILENO);
			dup2(sfds[1], STDOUT_FILENO);
			assert(execl(procPath, procName, NULL) == 0);
		}
		else // parent
		{
			close(cfds[0]);
			close(sfds[1]);
			readFd_ = sfds[0];
			writeFd_ = cfds[1];
		}
	}

	void send(const std::string& str) const
	{
		write(writeFd_, str.c_str(), str.size());
	}

	void read(std::string& str)
	{
		int size = BUFSIZ * BUFSIZ;
		std::vector<char> buf(size);
		char* cur = &*buf.begin();
		int nread = 0;
		do
		{
			int n = ::read(readFd_, cur, size);
			nread += n;
			cur += n;
			size -= n;
		} while (buf[nread - 1] != '\n');
		str.append(buf.begin(), buf.begin() + nread);
	}

	std::string moveCommand_;
	std::string displayCommand_;
	int readFd_;
	int writeFd_;
};
