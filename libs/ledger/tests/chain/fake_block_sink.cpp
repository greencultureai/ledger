//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "fake_block_sink.hpp"

#include "ledger/chain/block.hpp"

void FakeBlockSink::OnBlock(fetch::ledger::Block const &block)
{
  block_queue_.push_back(block);
}

FakeBlockSink::BlockQueue const &FakeBlockSink::queue()
{
  return block_queue_;
}

void FakeBlockSink::clear()
{
  block_queue_.clear();
}
