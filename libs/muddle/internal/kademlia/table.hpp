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

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/sha1.hpp"
#include "kademlia/bucket.hpp"
#include "kademlia/primitives.hpp"
#include "moment/clock_interfaces.hpp"
#include "muddle/packet.hpp"

#include <fstream>

namespace fetch {
namespace muddle {

class NetworkId;

class KademliaTable
{
public:
  static constexpr uint64_t KADEMLIA_MAX_ID_BITS = KademliaAddress::KADEMLIA_MAX_ID_BITS;

  using Buckets        = std::array<Bucket, KADEMLIA_MAX_ID_BITS + 1>;
  using Peers          = std::deque<PeerInfo>;
  using Address        = Packet::Address;
  using PeerInfoPtr    = std::shared_ptr<PeerInfo>;
  using PeerMap        = std::unordered_map<Address, PeerInfoPtr>;
  using Uri            = network::Uri;
  using UriToPeerMap   = std::unordered_map<Uri, PeerInfoPtr>;
  using Port           = uint16_t;
  using PortList       = std::vector<Port>;
  using ConstByteArray = byte_array::ConstByteArray;
  using AddressSet     = std::unordered_set<Address>;

  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

  KademliaTable(Address const &own_address, NetworkId const &network);

  /// Construtors & destructors
  /// @{
  KademliaTable(KademliaTable const &) = delete;
  KademliaTable(KademliaTable &&)      = delete;
  ~KademliaTable()                     = default;
  /// @}

  /// Operators
  /// @{
  KademliaTable &operator=(KademliaTable const &) = delete;
  KademliaTable &operator=(KademliaTable &&) = delete;
  /// @}

  /// Kademlia
  /// @{
  ConstByteArray Ping(Address const &address, PortList port);
  Peers          FindPeer(Address const &address);
  Peers          FindPeer(Address const &address, uint64_t log_id, bool scan_left = true,
                          bool scan_right = true);
  Peers          FindPeerByHamming(Address const &address);
  Peers FindPeerByHamming(Address const &address, uint64_t hamming_id, bool scan_left = true,
                          bool scan_right = true);
  void ReportLiveliness(Address const &address, Address const &reporter, PeerInfo const &info = {});
  void ReportExistence(PeerInfo const &info, Address const &reporter);
  void ReportFailure(Address const &address, Address const &reporter);
  PeerInfoPtr GetPeerDetails(Address const &address);
  bool        HasUri(Uri const &uri) const;
  Address     GetAddressFromUri(Uri const &uri) const;
  std::size_t size() const;
  Uri         GetUri(Address const &address);
  std::size_t active_buckets() const;
  uint64_t    first_non_empty_bucket() const;
  /// @}

  /// For connection maintenance
  /// @{
  Peers ProposePermanentConnections() const;
  /// @}

  /// Storage of peer table.
  /// @{
  void Dump();
  void Load();
  void SetCacheFile(std::string const &filename);
  /// @}
protected:
  friend class PeerTracker;

  /// Methods to manage desired peers
  /// @{
  void                    ClearDesired();
  void                    TrimDesiredPeers();
  void                    ConvertDesiredUrisToAddresses();
  std::unordered_set<Uri> desired_uris() const;
  AddressSet              desired_peers() const;
  void                    AddDesiredPeer(Address const &address, Duration const &expiry);
  void AddDesiredPeer(Address const &address, network::Peer const &hint, Duration const &expiry);
  void AddDesiredPeer(Uri const &uri, Duration const &expiry);
  void RemoveDesiredPeer(Address const &address);
  /// @}

private:
  Peers FindPeerInternal(KademliaAddress const &kam_address, uint64_t log_id, bool scan_left = true,
                         bool scan_right = true);
  Peers FindPeerByHammingInternal(KademliaAddress const &kam_address, uint64_t hamming_id,
                                  bool scan_left = true, bool scan_right = true);
  mutable std::mutex mutex_;
  std::string const  logging_name_;
  Address            own_address_;
  KademliaAddress    own_kad_address_;
  Buckets            by_logarithm_;
  Buckets            by_hamming_;
  PeerMap            know_peers_;
  UriToPeerMap       known_uris_;

  uint64_t first_non_empty_bucket_{KADEMLIA_MAX_ID_BITS};
  uint64_t kademlia_max_peers_per_bucket_{20};

  /// User defined connections
  /// @{
  mutable std::mutex desired_mutex_;  ///< Use to protect desired peer variables
                                      /// peers to avoid causing a deadlock
  std::unordered_map<Address, Timepoint> connection_expiry_;
  std::unordered_map<Uri, Timepoint>     desired_uri_expiry_;
  AddressSet                             desired_peers_;
  std::unordered_set<Uri>                desired_uris_;
  /// @}

  /// Backup
  /// @{
  std::string filename_{};
  /// @}

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

}  // namespace muddle

namespace serializers {

template <typename D>
struct MapSerializer<muddle::KademliaTable, D>
{
public:
  using Type       = muddle::KademliaTable;
  using DriverType = D;

  static uint8_t const BY_LOGARITHM      = 1;
  static uint8_t const BY_HAMMING        = 2;
  static uint8_t const KNOWN_PEERS       = 3;
  static uint8_t const KNOWN_URIS        = 4;
  static uint8_t const CONNECTION_EXPIRY = 5;
  static uint8_t const DESIRED_EXPIRY    = 6;
  static uint8_t const DESIRED_PEERS     = 7;
  static uint8_t const DESIRED_URIS      = 8;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &item)
  {
    auto map = map_constructor(8);

    map.Append(BY_LOGARITHM, item.by_logarithm_);
    map.Append(BY_HAMMING, item.by_hamming_);
    map.Append(KNOWN_PEERS, item.know_peers_);
    map.Append(KNOWN_URIS, item.known_uris_);
    map.Append(CONNECTION_EXPIRY, item.connection_expiry_);
    map.Append(DESIRED_EXPIRY, item.desired_uri_expiry_);
    map.Append(DESIRED_PEERS, item.desired_peers_);
    map.Append(DESIRED_URIS, item.desired_uris_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &item)
  {
    map.ExpectKeyGetValue(BY_LOGARITHM, item.by_logarithm_);
    map.ExpectKeyGetValue(BY_HAMMING, item.by_hamming_);
    map.ExpectKeyGetValue(KNOWN_PEERS, item.know_peers_);
    map.ExpectKeyGetValue(KNOWN_URIS, item.known_uris_);
    map.ExpectKeyGetValue(CONNECTION_EXPIRY, item.connection_expiry_);
    map.ExpectKeyGetValue(DESIRED_EXPIRY, item.desired_uri_expiry_);
    map.ExpectKeyGetValue(DESIRED_PEERS, item.desired_peers_);
    map.ExpectKeyGetValue(DESIRED_URIS, item.desired_uris_);
  }
};
}  // namespace serializers
}  // namespace fetch
