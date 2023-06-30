#include "muduo-websocket/WebsocketServer.h"
#include <muduo/net/EventLoop.h>
#include "UcciEngine.h"
#include "../c-cchess-update/utils.h"
#include "../c-cchess-update/board.h"
#include "../c-cchess-update/search_engine.h"
#include "../cpp-cchess-update/board.h"
#include "../cpp-cchess-update/search_engine.h"
#include "../cpp-cchess-update/utils.h"
#include <json/json.h>

using muduo::net::EventLoop;
using muduo::net::InetAddress;
using muduo::net::websocket::WebsocketServer;
using muduo::net::websocket::WebsocketConnectionPtr;
using wsun::cchess::cppupdate::Board;
using wsun::cchess::cppupdate::SideType;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

struct ProtocolType
{
  int type;
};

struct EnginePlayer
{
  int side;
  int stepSearchTime;
  EnginePlayer(int side, int stepSearchTime)
    : side(side), stepSearchTime(stepSearchTime) {}

  virtual const std::string search(const std::string& fen) = 0;
  virtual const std::string getFen()
  {
    return "";
  }
};

struct CEnginePlayer : EnginePlayer
{
  struct board* board;
  struct search_engine* engine;
  CEnginePlayer(int side, int stepSearchTime)
    : EnginePlayer(side, stepSearchTime),
      board(board_create(side)),
      engine(search_engine_create(board)) {}

  ~CEnginePlayer()
  { 
    search_engine_release(engine);
    board_release(board);
  }

  const std::string search(const std::string& fen)
  {
    board_reset_from_fen(board, fen.c_str());
    int mv = ::search(engine, stepSearchTime);
    if (mv == 0) return "";
    char iccs_mv[5] = {0};
    move_to_iccs_move(iccs_mv, mv);
    return iccs_mv;
  }

  const std::string getFen()
  {
    char fen[512] = {0};
    board_to_fen(board, fen);
    return fen;
  }
};

using wsun::cchess::cppupdate::Board;
using wsun::cchess::cppupdate::SearchEngine;

struct CCEnginePlayer : EnginePlayer
{
  std::unique_ptr<Board> board;
  std::unique_ptr<SearchEngine> engine;
  CCEnginePlayer(int side, int stepSearchTime)
    : EnginePlayer(side, stepSearchTime),
      board(new Board((SideType)side)),
      engine(new SearchEngine(board.get()))
  {
  }

  const std::string search(const std::string& fen)
  {
    board->resetFromFen(fen.c_str());
    int mv = engine->search(stepSearchTime);
    if (mv == 0) return "";
    char iccs_mv[5] = {0};
    move_to_iccs_move(iccs_mv, mv);
    return iccs_mv;
  }

  const std::string getFen()
  {
    return board->toFen();
  }
};

struct StockfishEnginePlayer : EnginePlayer
{
  std::unique_ptr<UcciEngine> engine;
  StockfishEnginePlayer(int side, int stepSearchTime)
    : EnginePlayer(side, stepSearchTime),
			engine(new UcciEngine("stockfish", "./stockfish/stockfish", "go movetime", "d"))
  {
  }

  const std::string search(const std::string& fen)
  {
    return engine->search(fen, stepSearchTime);
  }
};

struct EleeyeEnginePlayer : EnginePlayer
{
  std::unique_ptr<UcciEngine> engine;
  EleeyeEnginePlayer(int side, int stepSearchTime)
    : EnginePlayer(side, stepSearchTime),
			engine(new UcciEngine("stockfish", "./eleeye/eleeye", "go time", "d"))
  {
  }

  const std::string search(const std::string& fen)
  {
    return engine->search(fen, stepSearchTime);
  }
};

struct MatchContext
{
  std::unique_ptr<Board> board;
  std::unique_ptr<EnginePlayer> engines[2];
  int side;

  MatchContext()
  {
    board.reset(new Board);
    initEngine();
  }

  void initEngine()
  {
    side = 0;
    engines[0].reset(new CEnginePlayer(SIDE_TYPE_RED, 1000)); 
    engines[1].reset(new CCEnginePlayer(SIDE_TYPE_BLACK, 1000)); 
  }

  std::string play()
  {
    auto curFen = board->toFen();
    auto iccsMv = engines[side]->search(curFen);
    if (iccsMv.empty()) return iccsMv;
    board->play(iccsMv.c_str());
    side = 1 - side;
    return iccsMv;
  }

  std::string getCurrentFen()
  {
    return board->toFen();
  }
};

class OnlineStockFishEngine
{
public:
	OnlineStockFishEngine(EventLoop* loop, const InetAddress& addr)
		: server_(loop, addr, "onlineEngine")
	{
		server_.setConnectionCallback(
			std::bind(&OnlineStockFishEngine::onConnection, this, _1));
		server_.setTextMessageCallback(
			std::bind(&OnlineStockFishEngine::onMessage, this, _1, _2));
	}

	void start() { server_.start(); }
private:
	void onConnection(const WebsocketConnectionPtr& conn)
	{
		if (!conn->connected())
		{
			const boost::any& context = conn->getContext();
			if (!context.empty())
			{
				MatchContext* ctx = boost::any_cast<MatchContext*>(context);
				delete ctx;
				conn->setContext(nullptr);
			}
		}
	}

	void sendMoveStep(const WebsocketConnectionPtr& conn, const std::string& mv, const std::string& fen)
	{
    Json::Value innerData;
		innerData["mv"] = mv;
		innerData["fen"] = fen;
		Json::Value root;
		root["type"] = 5;
    root["data"] = innerData;

		auto data = Json::FastWriter().write(root);
		conn->sendText(data);
	}

  void handleProtocol(const WebsocketConnectionPtr& conn, int type, const Json::Value& json)
  {
    if (type == 1)
    {
      if (context_) {}
      else
      {
        context_.reset(new MatchContext);
        auto iccs_mv = context_->play();
        while (!iccs_mv.empty())
        {
          auto fen = context_->getCurrentFen();
          sendMoveStep(conn, iccs_mv, fen);
          iccs_mv = context_->play();
        }
      }
    }
    else if (type == 5)
    {
    }
  }

	void onMessage(const WebsocketConnectionPtr& conn, const std::string& msg)
	{
		Json::Value root;
		std::string err;
		Json::CharReaderBuilder().newCharReader()->parse(&*msg.begin(), &*msg.end(), &root, &err);
    int protocolType = root["type"].asInt();
    handleProtocol(conn, protocolType, root);
	}

  std::unique_ptr<MatchContext> context_;
	WebsocketServer server_;
};

int main(int argc, char *argv[])
{
	EventLoop loop;
	OnlineStockFishEngine engine(&loop, InetAddress(atoi(argv[1])));
	engine.start();
	loop.loop();
}
