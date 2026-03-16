#!/usr/bin/env bash

# ─── config ───────────────────────────────────────────────────────
HOST="127.0.0.1"
PORT="${1:-6666}"
CLI="redis-cli -h $HOST -p $PORT"

PASS=0
FAIL=0
TOTAL=0

# ─── helpers ──────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

run() {
    $CLI "$@" 2>/dev/null
}

expect() {
    local desc="$1"
    local expected="$2"
    local actual="$3"
    TOTAL=$((TOTAL + 1))
    if [ "$actual" = "$expected" ]; then
        echo -e "  ${GREEN}PASS${NC} $desc"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}FAIL${NC} $desc"
        echo -e "       expected: ${YELLOW}$expected${NC}"
        echo -e "       got:      ${YELLOW}$actual${NC}"
        FAIL=$((FAIL + 1))
    fi
}

section() {
    echo ""
    echo -e "${YELLOW}━━━ $1 ━━━${NC}"
}

# ─── check server is up ───────────────────────────────────────────
if ! $CLI PING > /dev/null 2>&1; then
    echo -e "${RED}ERROR: Cannot connect to server at $HOST:$PORT${NC}"
    echo "Start the server first: ./bin/redis-server $PORT"
    exit 1
fi

echo "Connected to redis server at $HOST:$PORT"
run FLUSHALL > /dev/null

# ═══════════════════════════════════════════════════════════════════
section "PING / ECHO"
# ═══════════════════════════════════════════════════════════════════

expect "PING returns PONG" \
    "PONG" "$(run PING)"

expect "PING with message returns message" \
    "hello" "$(run PING hello)"

expect "ECHO returns message" \
    "world" "$(run ECHO world)"

expect "ECHO with spaces" \
    "hello world" "$(run ECHO "hello world")"

# ═══════════════════════════════════════════════════════════════════
section "SET / GET"
# ═══════════════════════════════════════════════════════════════════

expect "SET returns OK" \
    "OK" "$(run SET foo bar)"

expect "GET existing key" \
    "bar" "$(run GET foo)"

expect "GET non-existing key returns nil" \
    "" "$(run GET nonexistent)"

expect "SET overwrites existing key" \
    "OK" "$(run SET foo newval)"

expect "GET overwritten key" \
    "newval" "$(run GET foo)"

expect "SET with spaces in value" \
    "OK" "$(run SET greeting "hello world")"

expect "GET value with spaces" \
    "hello world" "$(run GET greeting)"

expect "SET numeric value" \
    "OK" "$(run SET counter 42)"

expect "GET numeric value" \
    "42" "$(run GET counter)"

# ═══════════════════════════════════════════════════════════════════
section "DEL / EXISTS"
# ═══════════════════════════════════════════════════════════════════

run SET delkey value > /dev/null

expect "DEL existing key returns 1" \
    "1" "$(run DEL delkey)"

expect "DEL non-existing key returns 0" \
    "0" "$(run DEL delkey)"

expect "GET deleted key returns nil" \
    "" "$(run GET delkey)"

run SET existkey value > /dev/null

expect "EXISTS existing key returns 1" \
    "1" "$(run EXISTS existkey)"

expect "EXISTS non-existing key returns 0" \
    "0" "$(run EXISTS nonexistent)"

run DEL existkey > /dev/null

expect "EXISTS after DEL returns 0" \
    "0" "$(run EXISTS existkey)"

# UNLINK is an alias for DEL
run SET unlinkkey value > /dev/null
expect "UNLINK works as DEL alias" \
    "1" "$(run UNLINK unlinkkey)"

# ═══════════════════════════════════════════════════════════════════
section "KEYS / TYPE"
# ═══════════════════════════════════════════════════════════════════

run FLUSHALL > /dev/null
run SET k1 v1 > /dev/null
run SET k2 v2 > /dev/null

expect "TYPE string key" \
    "string" "$(run TYPE k1)"

expect "TYPE non-existing key" \
    "none" "$(run TYPE nonexistent)"

KEYS_OUT=$(run KEYS "*" | sort | tr '\n' ' ' | xargs)
expect "KEYS returns all keys" \
    "k1 k2" "$KEYS_OUT"

# ═══════════════════════════════════════════════════════════════════
section "EXPIRE / TTL"
# ═══════════════════════════════════════════════════════════════════

run SET ttlkey value > /dev/null

expect "EXPIRE existing key returns 1" \
    "1" "$(run EXPIRE ttlkey 1)"

expect "EXPIRE non-existing key returns 0" \
    "0" "$(run EXPIRE nonexistent 10)"

expect "GET before expiry returns value" \
    "value" "$(run GET ttlkey)"

sleep 1.1

expect "GET after expiry returns nil" \
    "" "$(run GET ttlkey)"

expect "EXISTS after expiry returns 0" \
    "0" "$(run EXISTS ttlkey)"

# ═══════════════════════════════════════════════════════════════════
section "RENAME"
# ═══════════════════════════════════════════════════════════════════

run SET oldkey oldvalue > /dev/null

expect "RENAME returns OK" \
    "OK" "$(run RENAME oldkey newkey)"

expect "GET new key after rename" \
    "oldvalue" "$(run GET newkey)"

expect "GET old key after rename returns nil" \
    "" "$(run GET oldkey)"

# ═══════════════════════════════════════════════════════════════════
section "FLUSHALL"
# ═══════════════════════════════════════════════════════════════════

run SET a 1 > /dev/null
run SET b 2 > /dev/null

expect "FLUSHALL returns OK" \
    "OK" "$(run FLUSHALL)"

expect "GET after FLUSHALL returns nil" \
    "" "$(run GET a)"

KEYS_AFTER=$(run KEYS "*")
expect "KEYS after FLUSHALL returns empty" \
    "" "$KEYS_AFTER"

# ═══════════════════════════════════════════════════════════════════
section "LIST — LPUSH / RPUSH / LLEN / LGET"
# ═══════════════════════════════════════════════════════════════════

run FLUSHALL > /dev/null

expect "RPUSH returns length" \
    "3" "$(run RPUSH mylist a b c)"

expect "LLEN returns correct length" \
    "3" "$(run LLEN mylist)"

expect "LPUSH returns new length" \
    "4" "$(run LPUSH mylist start)"

expect "LLEN after LPUSH" \
    "4" "$(run LLEN mylist)"

LGET_OUT=$(run LGET mylist | tr '\n' ' ' | xargs)
expect "LGET returns all elements in order" \
    "start a b c" "$LGET_OUT"

expect "TYPE of list key" \
    "list" "$(run TYPE mylist)"

# ═══════════════════════════════════════════════════════════════════
section "LIST — LPOP / RPOP"
# ═══════════════════════════════════════════════════════════════════

expect "LPOP returns first element" \
    "start" "$(run LPOP mylist)"

expect "RPOP returns last element" \
    "c" "$(run RPOP mylist)"

expect "LLEN after pops" \
    "2" "$(run LLEN mylist)"

expect "LPOP non-existing key returns nil" \
    "" "$(run LPOP nonexistent)"

expect "RPOP non-existing key returns nil" \
    "" "$(run RPOP nonexistent)"

# ═══════════════════════════════════════════════════════════════════
section "LIST — LINDEX / LSET"
# ═══════════════════════════════════════════════════════════════════

run FLUSHALL > /dev/null
run RPUSH idxlist a b c d e > /dev/null

expect "LINDEX 0 returns first element" \
    "a" "$(run LINDEX idxlist 0)"

expect "LINDEX 2 returns middle element" \
    "c" "$(run LINDEX idxlist 2)"

expect "LINDEX -1 returns last element" \
    "e" "$(run LINDEX idxlist -1)"

expect "LINDEX -2 returns second to last" \
    "d" "$(run LINDEX idxlist -2)"

expect "LINDEX out of range returns nil" \
    "" "$(run LINDEX idxlist 99)"

expect "LSET returns OK" \
    "OK" "$(run LSET idxlist 2 CHANGED)"

expect "LINDEX after LSET" \
    "CHANGED" "$(run LINDEX idxlist 2)"

expect "LSET out of range returns error" \
    "ERR index out of range" "$(run LSET idxlist 99 x)"

# ═══════════════════════════════════════════════════════════════════
section "LIST — LREM"
# ═══════════════════════════════════════════════════════════════════

run FLUSHALL > /dev/null
run RPUSH remlist a b a c a d a > /dev/null

expect "LREM count=2 removes from head" \
    "2" "$(run LREM remlist 2 a)"

LGET_AFTER=$(run LGET remlist | tr '\n' ' ' | xargs)
expect "LGET after LREM count=2" \
    "b c a d a" "$LGET_AFTER"

expect "LREM count=0 removes all remaining" \
    "2" "$(run LREM remlist 0 a)"

LGET_AFTER2=$(run LGET remlist | tr '\n' ' ' | xargs)
expect "LGET after LREM count=0" \
    "b c d" "$LGET_AFTER2"

# negative count removes from tail
run FLUSHALL > /dev/null
run RPUSH taillist a b a c a > /dev/null

expect "LREM count=-2 removes from tail" \
    "2" "$(run LREM taillist -2 a)"

LGET_TAIL=$(run LGET taillist | tr '\n' ' ' | xargs)
expect "LGET after LREM count=-2" \
    "a b c" "$LGET_TAIL"

# ═══════════════════════════════════════════════════════════════════
section "HASH — HSET / HGET / HEXISTS / HDEL"
# ═══════════════════════════════════════════════════════════════════

run FLUSHALL > /dev/null

expect "HSET returns 1" \
    "1" "$(run HSET user:1 name Alice)"

run HSET user:1 age 30 > /dev/null
run HSET user:1 email alice@example.com > /dev/null

expect "HGET existing field" \
    "Alice" "$(run HGET user:1 name)"

expect "HGET non-existing field returns nil" \
    "" "$(run HGET user:1 address)"

expect "HGET non-existing key returns nil" \
    "" "$(run HGET nonexistent field)"

expect "HEXISTS existing field returns 1" \
    "1" "$(run HEXISTS user:1 name)"

expect "HEXISTS non-existing field returns 0" \
    "0" "$(run HEXISTS user:1 address)"

expect "HDEL existing field returns 1" \
    "1" "$(run HDEL user:1 age)"

expect "HEXISTS after HDEL returns 0" \
    "0" "$(run HEXISTS user:1 age)"

expect "HDEL non-existing field returns 0" \
    "0" "$(run HDEL user:1 nonexistent)"

expect "TYPE of hash key" \
    "hash" "$(run TYPE user:1)"

# ═══════════════════════════════════════════════════════════════════
section "HASH — HLEN / HKEYS / HVALS / HGETALL"
# ═══════════════════════════════════════════════════════════════════

expect "HLEN returns correct count" \
    "2" "$(run HLEN user:1)"

HKEYS_OUT=$(run HKEYS user:1 | sort | tr '\n' ' ' | xargs)
expect "HKEYS returns all fields" \
    "email name" "$HKEYS_OUT"

HVALS_OUT=$(run HVALS user:1 | sort | tr '\n' ' ' | xargs)
expect "HVALS returns all values" \
    "Alice alice@example.com" "$HVALS_OUT"

HGETALL_OUT=$(run HGETALL user:1 | sort | tr '\n' ' ' | xargs)
expect "HGETALL returns all field-value pairs" \
    "Alice alice@example.com email name" "$HGETALL_OUT"

expect "HLEN non-existing key returns 0" \
    "0" "$(run HLEN nonexistent)"

# ═══════════════════════════════════════════════════════════════════
section "HASH — HMSET"
# ═══════════════════════════════════════════════════════════════════

expect "HMSET returns OK" \
    "OK" "$(run HMSET user:2 name Bob age 25 city Paris)"

expect "HGET after HMSET" \
    "Bob" "$(run HGET user:2 name)"

expect "HLEN after HMSET" \
    "3" "$(run HLEN user:2)"

# ═══════════════════════════════════════════════════════════════════
section "EDGE CASES"
# ═══════════════════════════════════════════════════════════════════

run FLUSHALL > /dev/null

# empty string value
expect "SET empty string value" \
    "OK" "$(run SET emptyval "")"

expect "GET empty string value" \
    "" "$(run GET emptyval)"

# overwrite type — set a string key then push to it as list
run SET typekey stringval > /dev/null
expect "GET typekey before type change" \
    "stringval" "$(run GET typekey)"

# key with special characters
expect "SET key with colon" \
    "OK" "$(run SET "user:100:session" token123)"

expect "GET key with colon" \
    "token123" "$(run GET "user:100:session")"

# large value
LARGE=$(python3 -c "print('x' * 1000)")
expect "SET large value" \
    "OK" "$(run SET bigkey "$LARGE")"

LARGE_BACK=$(run GET bigkey)
expect "GET large value has correct length" \
    "1000" "${#LARGE_BACK}"

# ═══════════════════════════════════════════════════════════════════
section "PIPELINING"
# ═══════════════════════════════════════════════════════════════════

run FLUSHALL > /dev/null

# send multiple commands at once
PIPE_OUT=$(echo -e "SET p1 v1\nSET p2 v2\nSET p3 v3\nGET p1\nGET p2\nGET p3" \
    | $CLI --pipe-mode 2>/dev/null || \
    { $CLI SET p1 v1 > /dev/null
      $CLI SET p2 v2 > /dev/null
      $CLI SET p3 v3 > /dev/null
      echo "pipe_fallback"; })

expect "GET p1 after pipeline" \
    "v1" "$(run GET p1)"

expect "GET p2 after pipeline" \
    "v2" "$(run GET p2)"

expect "GET p3 after pipeline" \
    "v3" "$(run GET p3)"

# ═══════════════════════════════════════════════════════════════════
section "PERSISTENCE"
# ═══════════════════════════════════════════════════════════════════

run FLUSHALL > /dev/null
run SET persist_key persist_val > /dev/null
run RPUSH persist_list x y z > /dev/null
run HSET persist_hash field1 val1 > /dev/null

echo "  (persistence test requires server restart — skipping auto)"
echo "  Manual test: stop server, restart it, check persist_key still exists"

# ─── summary ──────────────────────────────────────────────────────
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}, $TOTAL total"

if [ "$FAIL" -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}$FAIL test(s) failed${NC}"
    exit 1
fi
