// Copyright (c) 2015 The Tin Project. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include <limits.h>
#include <utility>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/stl_util.h"

#include "tin/net/inet.h"
#include "tin/net/ip_address.h"


namespace {

// The prefix for IPv6 mapped IPv4 addresses.
// https://tools.ietf.org/html/rfc4291#section-2.5.5.2
const uint8 kIPv4MappedPrefix[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};

// Note that this function assumes:
// * |ip_address| is at least |prefix_length_in_bits| (bits) long;
// * |ip_prefix| is at least |prefix_length_in_bits| (bits) long.
bool IPAddressPrefixCheck(const std::vector<uint8>& ip_address,
                          const uint8* ip_prefix,
                          size_t prefix_length_in_bits) {
  // Compare all the bytes that fall entirely within the prefix.
  size_t num_entire_bytes_in_prefix = prefix_length_in_bits / 8;
  for (size_t i = 0; i < num_entire_bytes_in_prefix; ++i) {
    if (ip_address[i] != ip_prefix[i])
      return false;
  }

  // In case the prefix was not a multiple of 8, there will be 1 byte
  // which is only partially masked.
  size_t remaining_bits = prefix_length_in_bits % 8;
  if (remaining_bits != 0) {
    uint8 mask = 0xFF << (8 - remaining_bits);
    size_t i = num_entire_bytes_in_prefix;
    if ((ip_address[i] & mask) != (ip_prefix[i] & mask))
      return false;
  }
  return true;
}

// Returns true if |ip_address| matches any of the reserved IPv4 ranges. This
// method operates on a blacklist of reserved IPv4 ranges. Some ranges are
// consolidated.
// Sources for info:
// www.iana.org/assignments/ipv4-address-space/ipv4-address-space.xhtml
// www.iana.org/assignments/iana-ipv4-special-registry/
// iana-ipv4-special-registry.xhtml

namespace {
struct ReservedIPv4Range {
  const uint8 address[4];
  size_t prefix_length_in_bits;
};
}
bool IsReservedIPv4(const std::vector<uint8>& ip_address) {
  // Different IP versions have different range reservations.
  DCHECK_EQ(tin::net::IPAddress::kIPv4AddressSize, ip_address.size());
  static const ReservedIPv4Range kReservedIPv4Ranges[] = {
    {{0, 0, 0, 0}, 8},     {{10, 0, 0, 0}, 8},      {{100, 64, 0, 0}, 10},
    {{127, 0, 0, 0}, 8},   {{169, 254, 0, 0}, 16},  {{172, 16, 0, 0}, 12},
    {{192, 0, 2, 0}, 24},  {{192, 88, 99, 0}, 24},  {{192, 168, 0, 0}, 16},
    {{198, 18, 0, 0}, 15}, {{198, 51, 100, 0}, 24}, {{203, 0, 113, 0}, 24},
    {{224, 0, 0, 0}, 3}
  };

  for (unsigned i = 0; i < arraysize(kReservedIPv4Ranges); ++i) {
    if (IPAddressPrefixCheck(ip_address, kReservedIPv4Ranges[i].address,
                             kReservedIPv4Ranges[i].prefix_length_in_bits)) {
      return true;
    }
  }

  return false;
}

// Returns true if |ip_address| matches any of the reserved IPv6 ranges. This
// method operates on a whitelist of non-reserved IPv6 ranges. All IPv6
// addresses outside these ranges are reserved.
// Sources for info:
// www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xhtml

namespace {
struct PublicIPv6Range {
  const uint8 address_prefix[2];
  size_t prefix_length_in_bits;
};
}

bool IsReservedIPv6(const std::vector<uint8>& ip_address) {
  // Different IP versions have different range reservations.
  DCHECK_EQ(tin::net::IPAddress::kIPv6AddressSize, ip_address.size());
  static const PublicIPv6Range kPublicIPv6Ranges[] = {
    // 2000::/3  -- Global Unicast
    {{0x20, 0}, 3},
    // ff00::/8  -- Multicast
    {{0xff, 0}, 8},
  };

  for (unsigned i = 0; i < arraysize(kPublicIPv6Ranges); ++i) {
    if (IPAddressPrefixCheck(ip_address, kPublicIPv6Ranges[i].address_prefix,
                             kPublicIPv6Ranges[i].prefix_length_in_bits)) {
      return false;
    }
  }

  return true;
}

bool ParseIPLiteralToBytes(const base::StringPiece& ip_literal,
                           std::vector<uint8>* bytes) {
  // |ip_literal| could be either an IPv4 or an IPv6 literal. If it contains
  // a colon however, it must be an IPv6 address.
  bool ipv4 = ip_literal.find(':') == base::StringPiece::npos;
  bytes->resize(ipv4 ? 4 : 16);  // 128 bits.
  return tin::net::INetPToN(ipv4, ip_literal.data(), vector_as_array(bytes));
}

}  // namespace
namespace tin {
namespace net {

IPAddress::IPAddress() {}

IPAddress::IPAddress(const std::vector<uint8>& address)
  : ip_address_(address) {}

IPAddress::IPAddress(const uint8* address, size_t address_len)
  : ip_address_(address, address + address_len) {}

IPAddress::IPAddress(uint8 b0, uint8 b1, uint8 b2, uint8 b3) {
  ip_address_.reserve(4);
  ip_address_.push_back(b0);
  ip_address_.push_back(b1);
  ip_address_.push_back(b2);
  ip_address_.push_back(b3);
}

IPAddress::IPAddress(uint8 b0,
                     uint8 b1,
                     uint8 b2,
                     uint8 b3,
                     uint8 b4,
                     uint8 b5,
                     uint8 b6,
                     uint8 b7,
                     uint8 b8,
                     uint8 b9,
                     uint8 b10,
                     uint8 b11,
                     uint8 b12,
                     uint8 b13,
                     uint8 b14,
                     uint8 b15) {
  const uint8 address[] = {b0, b1, b2,  b3,  b4,  b5,  b6,  b7,
                           b8, b9, b10, b11, b12, b13, b14, b15
                          };
  ip_address_.assign(address, address + arraysize(address));
}


IPAddress::IPAddress(const IPAddress& other)
  : ip_address_(other.ip_address_) {}

IPAddress::~IPAddress() {}

bool IPAddress::IsIPv4() const {
  return ip_address_.size() == kIPv4AddressSize;
}

bool IPAddress::IsIPv6() const {
  return ip_address_.size() == kIPv6AddressSize;
}

bool IPAddress::IsValid() const {
  return IsIPv4() || IsIPv6();
}

bool IPAddress::IsReserved() const {
  if (IsIPv4()) {
    return IsReservedIPv4(ip_address_);
  } else if (IsIPv6()) {
    return IsReservedIPv6(ip_address_);
  }
  return false;
}

bool IPAddress::IsZero() const {
  for (unsigned i = 0; i < ip_address_.size(); ++i) {
    if (ip_address_[i] != 0)
      return false;
  }

  return !empty();
}

bool IPAddress::IsIPv4MappedIPv6() const {
  return IsIPv6() && IPAddressStartsWith(*this, kIPv4MappedPrefix);
}

bool IPAddress::AssignFromIPLiteral(const base::StringPiece& ip_literal) {
  std::vector<uint8> number;

  if (!ParseIPLiteralToBytes(ip_literal, &number))
    return false;

  std::swap(number, ip_address_);
  return true;
}

// static
IPAddress IPAddress::IPv4Localhost() {
  static const uint8 kLocalhostIPv4[] = {127, 0, 0, 1};
  return IPAddress(kLocalhostIPv4);
}

// static
IPAddress IPAddress::IPv6Localhost() {
  static const uint8 kLocalhostIPv6[] = {0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 1
                                        };
  return IPAddress(kLocalhostIPv6);
}

// static
IPAddress IPAddress::AllZeros(size_t num_zero_bytes) {
  return IPAddress(std::vector<uint8>(num_zero_bytes));
}

// static
IPAddress IPAddress::IPv4AllZeros() {
  return AllZeros(kIPv4AddressSize);
}

// static
IPAddress IPAddress::IPv6AllZeros() {
  return AllZeros(kIPv6AddressSize);
}

bool IPAddress::operator==(const IPAddress& that) const {
  return ip_address_ == that.ip_address_;
}

bool IPAddress::operator!=(const IPAddress& that) const {
  return ip_address_ != that.ip_address_;
}

bool IPAddress::operator<(const IPAddress& that) const {
  // Sort IPv4 before IPv6.
  if (ip_address_.size() != that.ip_address_.size()) {
    return ip_address_.size() < that.ip_address_.size();
  }

  return ip_address_ < that.ip_address_;
}

std::string IPAddress::ToString() const {
  std::string str;
  InetNToP(IsIPv4(), vector_as_array(&ip_address_), &str);
  return str;
}

std::string IPAddressToStringWithPort(const IPAddress& address, uint16 port) {
  std::string address_str = address.ToString();
  if (address_str.empty())
    return address_str;

  if (address.IsIPv6()) {
    // Need to bracket IPv6 addresses since they contain colons.
    return base::StringPrintf("[%s]:%d", address_str.c_str(), port);
  }
  return base::StringPrintf("%s:%d", address_str.c_str(), port);
}

std::string IPAddressToPackedString(const IPAddress& address) {
  return std::string(reinterpret_cast<const char*>(
                       vector_as_array(&address.bytes())), address.size());
}

IPAddress ConvertIPv4ToIPv4MappedIPv6(const IPAddress& address) {
  DCHECK(address.IsIPv4());
  // IPv4-mapped addresses are formed by:
  // <80 bits of zeros>  + <16 bits of ones> + <32-bit IPv4 address.
  std::vector<uint8> bytes;
  bytes.reserve(16);
  bytes.insert(bytes.end(), kIPv4MappedPrefix,
               kIPv4MappedPrefix + arraysize(kIPv4MappedPrefix));
  bytes.insert(bytes.end(), address.bytes().begin(), address.bytes().end());
  return IPAddress(bytes);
}

IPAddress ConvertIPv4MappedIPv6ToIPv4(const IPAddress& address) {
  DCHECK(address.IsIPv4MappedIPv6());

  return IPAddress(std::vector<uint8>(
                     address.bytes().begin() + arraysize(kIPv4MappedPrefix),
                     address.bytes().end()));
}

bool IPAddressMatchesPrefix(const IPAddress& ip_address,
                            const IPAddress& ip_prefix,
                            size_t prefix_length_in_bits) {
  // Both the input IP address and the prefix IP address should be either IPv4
  // or IPv6.
  DCHECK(ip_address.IsValid());
  DCHECK(ip_prefix.IsValid());

  DCHECK_LE(prefix_length_in_bits, ip_prefix.size() * 8);

  // In case we have an IPv6 / IPv4 mismatch, convert the IPv4 addresses to
  // IPv6 addresses in order to do the comparison.
  if (ip_address.size() != ip_prefix.size()) {
    if (ip_address.IsIPv4()) {
      return IPAddressMatchesPrefix(ConvertIPv4ToIPv4MappedIPv6(ip_address),
                                    ip_prefix, prefix_length_in_bits);
    }
    return IPAddressMatchesPrefix(ip_address,
                                  ConvertIPv4ToIPv4MappedIPv6(ip_prefix),
                                  96 + prefix_length_in_bits);
  }

  return IPAddressPrefixCheck(ip_address.bytes(),
                              vector_as_array(&ip_prefix.bytes()),
                              prefix_length_in_bits);
}

bool ParseURLHostnameToAddress(const base::StringPiece& hostname,
                               IPAddress* ip_address) {
  if (hostname.size() >= 2 && hostname.front() == '[' &&
      hostname.back() == ']') {
    // Strip the square brackets that surround IPv6 literals.
    base::StringPiece ip_literal =
      base::StringPiece(hostname).substr(1, hostname.size() - 2);
    return ip_address->AssignFromIPLiteral(ip_literal) && ip_address->IsIPv6();
  }

  return ip_address->AssignFromIPLiteral(hostname) && ip_address->IsIPv4();
}

unsigned CommonPrefixLength(const IPAddress& a1, const IPAddress& a2) {
  DCHECK_EQ(a1.size(), a2.size());
  for (size_t i = 0; i < a1.size(); ++i) {
    unsigned diff = a1.bytes()[i] ^ a2.bytes()[i];
    if (!diff)
      continue;
    for (unsigned j = 0; j < CHAR_BIT; ++j) {
      if (diff & (1 << (CHAR_BIT - 1)))
        return static_cast<unsigned>(i * CHAR_BIT + j);
      diff <<= 1;
    }
    NOTREACHED();
  }
  return static_cast<unsigned>(a1.size() * CHAR_BIT);
}

unsigned MaskPrefixLength(const IPAddress& mask) {
  std::vector<uint8> all_ones(mask.size(), 0xFF);
  return CommonPrefixLength(mask, IPAddress(all_ones));
}

}  // namespace net
}  // namespace tin

