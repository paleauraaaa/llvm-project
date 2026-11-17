// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

namespace game {
enum class Kind : unsigned short { Idle = 3, Active = 7 };

struct Base {
  int id;
};

struct Actor : Base {
  struct Position {
    float x;
    float y;
  };
  Position position;
  Kind kind;
  unsigned int enabled : 1;
  unsigned int team : 3;
  int values[3];
  Actor *next;
  int GetID() const;
  int GetTeam() volatile;
  void SetID(int value);
  static int count;
};

template <typename T> struct Box {
  T value;
};

int Actor::GetID() const { return id; }
int Actor::GetTeam() volatile { return team; }
void Actor::SetID(int value) { id = value; }
int Actor::count = 9;
Actor actor = {{42}, {1.5f, 2.5f}, Kind::Active, 1, 5, {10, 20, 30}, nullptr};
Box<Actor *> box = {&actor};

#pragma bitfield_order(lsb_to_msb)
struct ReverseBits {
  unsigned low : 3;
  signed neg : 5;
  unsigned high : 8;
};
#pragma bitfield_order(msb_to_lsb)
ReverseBits reversed = {5, -3, 0xa6};
} // namespace game

#line 100 "xbox360-pdb.cpp"
extern "C" int debug_entry(int value) {
  volatile int local = value + game::actor.GetID();
  return local + game::actor.values[1];
}
