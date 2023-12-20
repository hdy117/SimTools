#pragma once

#include <iostream>
#include <string>

class MsgGen {
public:
  MsgGen();
  virtual ~MsgGen();

  const std::string &getPayload() const;
  void genMsgPayload();

private:
  std::string payload_;
};