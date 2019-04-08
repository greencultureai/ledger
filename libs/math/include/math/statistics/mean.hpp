#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "core/assert.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Mean(ArrayType const &a, typename ArrayType::Type &ret)
{
  ret = typename ArrayType::Type(0);
  for (std::size_t j = 0; j < a.size(); ++j)
  {
    ret += a.At(j);
  }
  ret /= static_cast<typename ArrayType::Type>(a.size());
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, typename ArrayType::Type> Mean(ArrayType const &a)
{

  typename ArrayType::Type ret = 0;
  for (std::size_t j = 0; j < a.size(); ++j)
  {
    ret += a.At(j);
  }
  ret /= static_cast<typename ArrayType::Type>(a.size());
  return ret;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch