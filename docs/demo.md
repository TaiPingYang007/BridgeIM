# BridgeIM Demo Guide

This guide demonstrates the BridgeIM deliverable 1.0 runtime stack. The demo is semi-automated: the script starts the multi-process backend stack, and the business flow is driven manually through `ChatClient` so the behavior is easy to observe and explain.

## Runtime Topology

```text
ChatClient
  -> Nginx :8000
  -> ChatServer node A :6000
  -> ChatServer node B :6002

ChatServer
  -> Redis pub/sub
  -> MySQL chatserver
  -> mprpc Channel
      -> FriendService :7001
      -> OfflineMessageService :7000

ZooKeeper :2181
  -> RPC service registration/discovery
```

## Prerequisites

Prepare `.env` first:

```bash
cp .env.example .env
# Adjust MySQL / Redis / ZooKeeper values if needed.
```

Required dependency services:

- shared MySQL container: `mysql_db` on `127.0.0.1:3306`
- BridgeIM Redis + Nginx from root `compose.yaml`
- ZooKeeper from `third_party/mprpc/compose.yaml` (`mprpc-zookeeper` on `127.0.0.1:2181`)

The demo script starts Redis, Nginx, and `mprpc-zookeeper` if needed. It expects the shared MySQL container/database to already be prepared.

## Start the Demo Stack

From the project root:

```bash
./scripts/demo/bridgeim-demo.sh
```

In this local Claude/WSL environment, commands are usually run through `rtk`:

```bash
rtk ./scripts/demo/bridgeim-demo.sh
```

The script will:

1. load `.env`
2. ensure ZooKeeper, Redis, and Nginx are running
3. verify MySQL, ZooKeeper, and Redis ports are reachable
4. build BridgeIM via `scripts/build/all.sh`
5. start:
   - `OfflineMessageService` on RPC port `7000`
   - `FriendService` on RPC port `7001`
   - `ChatServer 0.0.0.0 6000`
   - `ChatServer 0.0.0.0 6002`
6. print client commands and wait for ENTER to clean up the background processes

Runtime logs:

- BridgeIM business logs: `logs/`
- demo process stdout/stderr: `logs/demo/`

## Run Clients

Open two additional terminals in the project root.

### Option A: Cluster Entry Through Nginx

```bash
./bin/ChatClient 127.0.0.1 8000
```

Nginx forwards clients to the two ChatServer nodes.

### Option B: Controlled Direct-Node Test

For a controlled cross-node test, open one client per node:

```bash
# terminal A
./bin/ChatClient 127.0.0.1 6000

# terminal B
./bin/ChatClient 127.0.0.1 6002
```

This makes it easier to prove Redis cross-node forwarding.

## Suggested Business Flow

Use two users, for example `alice` and `bob`.

1. Register two accounts from the client landing page:
   - choose `2. register`
   - record the generated user ids
2. Log in as both users from two clients.
3. From Alice's client, request Bob as friend:

   ```text
   addfriend:<bob_id>
   ```

4. From Bob's client, accept Alice's request. The client will also print the suggested accept command when it receives the friend request:

   ```text
   acceptfriend:<alice_id>
   ```

5. Send a one-to-one message:

   ```text
   chat:<bob_id>:hello from alice
   ```

6. Test offline message persistence:
   - log Bob out or close Bob's client
   - from Alice, send another message to Bob
   - log Bob back in
   - Bob should receive the offline message in the login response

7. Optional group flow:

   ```text
   creategroup:study:BridgeIM demo group
   addgroup:<group_id>
   acceptaddgroup:<userid>:<group_id>
   groupchat:<group_id>:hello group
   ```

## What to Point Out

- `ChatServer` is the access gateway. It owns live TCP connections and keeps the external JSON + newline protocol stable.
- `FriendService` handles friend existence, friend relationships, and friend-request RPC methods.
- `OfflineMessageService` handles offline-message insert/query/remove RPC methods.
- Redis handles cross-node message fanout between ChatServer nodes.
- ZooKeeper is used for RPC service registration/discovery, not for message delivery.
- Runtime logs are separated into `logs/`; executables stay in `bin/`.

## Stop the Demo

Press ENTER in the terminal running `scripts/demo/bridgeim-demo.sh`. The script stops the BridgeIM service processes it launched.

Docker dependency services are left running intentionally. Stop them separately if needed:

```bash
docker compose down
docker compose -f third_party/mprpc/compose.yaml down
```
