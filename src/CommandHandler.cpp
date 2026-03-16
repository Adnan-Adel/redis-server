#include "../include/CommandHandler.hpp"
#include <algorithm>

CommandHandler::CommandHandler(DataBase &db)
    : db(db) {}

// ─── encode helpers ───────────────────────────────────────────────
static std::string ok() {
    return "+OK\r\n";
}
static std::string pong() {
    return "+PONG\r\n";
}
static std::string null() {
    return "$-1\r\n";
}
static std::string err(const std::string &msg) {
    return "-ERR " + msg + "\r\n";
}
static std::string integer(ssize_t n) {
    return ":" + std::to_string(n) + "\r\n";
}

static std::string bulk(const std::string &s) {
    return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}

static std::string array(const std::vector<std::string> &items) {
    std::string out = "*" + std::to_string(items.size()) + "\r\n";
    for (auto &item : items) out += bulk(item);
    return out;
}

// ─── handlers ────────────────────────────────────────────────────

static std::string handlePing(const std::vector<std::string> &args) {
    return args.size() >= 2 ? bulk(args[1]) : pong();
}

static std::string handleEcho(const std::vector<std::string> &args) {
    if (args.size() < 2) return err("wrong number of arguments for ECHO");
    return bulk(args[1]);
}

static std::string handleSet(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for SET");
    db.set(args[1], args[2]);
    return ok();
}

static std::string handleGet(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for GET");
    std::string value;
    return db.get(args[1], value) ? bulk(value) : null();
}

static std::string handleDel(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for DEL");
    return integer(db.del(args[1]) ? 1 : 0);
}

static std::string handleExists(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for EXISTS");
    return integer(db.exists(args[1]) ? 1 : 0);
}

static std::string handleKeys(const std::vector<std::string> &args, DataBase &db) {
    return array(db.keys());
}

static std::string handleType(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for TYPE");
    return "+" + db.type(args[1]) + "\r\n";
}

static std::string handleExpire(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for EXPIRE");
    try {
        return integer(db.expire(args[1], std::stoi(args[2])) ? 1 : 0);
    } catch (...) { return err("value is not an integer"); }
}

static std::string handleRename(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for RENAME");
    return db.rename(args[1], args[2]) ? ok() : err("no such key");
}

static std::string handleFlushAll(DataBase &db) {
    db.flushAll();
    return ok();
}

// list
static std::string handleLpush(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for LPUSH");
    for (size_t i = 2; i < args.size(); i++) db.lpush(args[1], args[i]);
    return integer(db.llen(args[1]));
}

static std::string handleRpush(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for RPUSH");
    for (size_t i = 2; i < args.size(); i++) db.rpush(args[1], args[i]);
    return integer(db.llen(args[1]));
}

static std::string handleLpop(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for LPOP");
    std::string value;
    return db.lpop(args[1], value) ? bulk(value) : null();
}

static std::string handleRpop(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for RPOP");
    std::string value;
    return db.rpop(args[1], value) ? bulk(value) : null();
}

static std::string handleLget(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for LGET");
    return array(db.lget(args[1]));
}

static std::string handleLlen(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for LLEN");
    return integer(db.llen(args[1]));
}

static std::string handleLrem(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 4) return err("wrong number of arguments for LREM");
    try {
        return integer(db.lrem(args[1], std::stoi(args[2]), args[3]));
    } catch (...) { return err("value is not an integer"); }
}

static std::string handleLindex(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for LINDEX");
    try {
        std::string value;
        return db.lindex(args[1], std::stoi(args[2]), value) ? bulk(value) : null();
    } catch (...) { return err("value is not an integer"); }
}

static std::string handleLset(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 4) return err("wrong number of arguments for LSET");
    try {
        return db.lset(args[1], std::stoi(args[2]), args[3]) ? ok() : err("index out of range");
    } catch (...) { return err("value is not an integer"); }
}

// hash
static std::string handleHset(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 4) return err("wrong number of arguments for HSET");
    db.hset(args[1], args[2], args[3]);
    return integer(1);
}

static std::string handleHget(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for HGET");
    std::string value;
    return db.hget(args[1], args[2], value) ? bulk(value) : null();
}

static std::string handleHexists(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for HEXISTS");
    return integer(db.hexists(args[1], args[2]) ? 1 : 0);
}

static std::string handleHdel(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 3) return err("wrong number of arguments for HDEL");
    return integer(db.hdel(args[1], args[2]) ? 1 : 0);
}

static std::string handleHgetall(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for HGETALL");
    auto hash = db.hgetall(args[1]);
    std::string out = "*" + std::to_string(hash.size() * 2) + "\r\n";
    for (auto &[field, value] : hash) {
        out += bulk(field);
        out += bulk(value);
    }
    return out;
}

static std::string handleHkeys(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for HKEYS");
    return array(db.hkeys(args[1]));
}

static std::string handleHvals(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for HVALS");
    return array(db.hvals(args[1]));
}

static std::string handleHlen(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 2) return err("wrong number of arguments for HLEN");
    return integer(db.hlen(args[1]));
}

static std::string handleHmset(const std::vector<std::string> &args, DataBase &db) {
    if (args.size() < 4 || args.size() % 2 != 0)
        return err("wrong number of arguments for HMSET");
    std::vector<std::pair<std::string, std::string>> pairs;
    for (size_t i = 2; i < args.size(); i += 2)
        pairs.emplace_back(args[i], args[i + 1]);
    db.hmset(args[1], pairs);
    return ok();
}

// ─── dispatch ────────────────────────────────────────────────────

std::string CommandHandler::execute(const std::vector<std::string> &args) {
    if (args.empty()) return err("empty command");

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    // common
    if (cmd == "PING") return handlePing(args);
    if (cmd == "ECHO") return handleEcho(args);
    if (cmd == "FLUSHALL") return handleFlushAll(db);

    // strings
    if (cmd == "SET") return handleSet(args, db);
    if (cmd == "GET") return handleGet(args, db);
    if (cmd == "DEL" || cmd == "UNLINK") return handleDel(args, db);
    if (cmd == "EXISTS") return handleExists(args, db);
    if (cmd == "KEYS") return handleKeys(args, db);
    if (cmd == "TYPE") return handleType(args, db);
    if (cmd == "EXPIRE") return handleExpire(args, db);
    if (cmd == "RENAME") return handleRename(args, db);

    // lists
    if (cmd == "LPUSH") return handleLpush(args, db);
    if (cmd == "RPUSH") return handleRpush(args, db);
    if (cmd == "LPOP") return handleLpop(args, db);
    if (cmd == "RPOP") return handleRpop(args, db);
    if (cmd == "LGET") return handleLget(args, db);
    if (cmd == "LLEN") return handleLlen(args, db);
    if (cmd == "LREM") return handleLrem(args, db);
    if (cmd == "LINDEX") return handleLindex(args, db);
    if (cmd == "LSET") return handleLset(args, db);

    // hashes
    if (cmd == "HSET") return handleHset(args, db);
    if (cmd == "HGET") return handleHget(args, db);
    if (cmd == "HEXISTS") return handleHexists(args, db);
    if (cmd == "HDEL") return handleHdel(args, db);
    if (cmd == "HGETALL") return handleHgetall(args, db);
    if (cmd == "HKEYS") return handleHkeys(args, db);
    if (cmd == "HVALS") return handleHvals(args, db);
    if (cmd == "HLEN") return handleHlen(args, db);
    if (cmd == "HMSET") return handleHmset(args, db);

    return err("unknown command '" + args[0] + "'");
}
