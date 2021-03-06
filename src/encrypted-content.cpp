#include "encrypted-content.hpp"
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/util/concepts.hpp>

#include <boost/lexical_cast.hpp>

namespace ndn {
namespace gep {

BOOST_CONCEPT_ASSERT((boost::EqualityComparable<EncryptedContent>));
BOOST_CONCEPT_ASSERT((WireEncodable<EncryptedContent>));
BOOST_CONCEPT_ASSERT((WireDecodable<EncryptedContent>));
static_assert(std::is_base_of<ndn::tlv::Error, EncryptedContent::Error>::value,
              "EncryptedContent::Error must inherit from tlv::Error");

EncryptedContent::EncryptedContent()
  : m_type(-1)
  , m_hasKeyLocator(false)
{
}

EncryptedContent::EncryptedContent(tlv::AlgorithmTypeValue type, const KeyLocator& keyLocator, const ConstBufferPtr& payload)
  : m_type(type)
  , m_hasKeyLocator(true)
  , m_keyLocator(keyLocator)
  , m_payload(payload)
{
}

EncryptedContent::EncryptedContent(const Block& block)
{
  wireDecode(block);
}

void
EncryptedContent::setAlgorithmType(tlv::AlgorithmTypeValue type)
{
  m_wire.reset();
  m_type = type;
}

void
EncryptedContent::setKeyLocator(const KeyLocator& keyLocator)
{
  m_wire.reset();
  m_keyLocator = keyLocator;
  m_hasKeyLocator = true;
}

const KeyLocator&
EncryptedContent::getKeyLocator() const
{
  if (m_hasKeyLocator)
    return m_keyLocator;
  else
    throw Error("KeyLocator does not exist");
}

void
EncryptedContent::setPayload(const ConstBufferPtr& payload)
{
  m_wire.reset();
  m_payload = payload;
}

const ConstBufferPtr
EncryptedContent::getPayload() const
{
  return m_payload;
}

template<encoding::Tag TAG>
size_t
EncryptedContent::wireEncode(EncodingImpl<TAG>& block) const
{
  size_t totalLength = 0;

  totalLength += block.appendByteArrayBlock(tlv::EncryptedPayload, m_payload->buf(), m_payload->size());

  if (m_type != -1)
    totalLength += prependNonNegativeIntegerBlock(block, tlv::EncryptionAlgorithm, m_type);
  else
    throw Error("EncryptedContent does not have an encryption algorithm");

  if (m_hasKeyLocator)
    totalLength += m_keyLocator.wireEncode(block);
  else
    throw Error("EncryptedContent does not have key locator");

  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::EncryptedContent);
  return totalLength;
}

const Block&
EncryptedContent::wireEncode() const
{
  if (m_wire.hasWire())
    return m_wire;

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();
  return m_wire;
}

void
EncryptedContent::wireDecode(const Block& wire)
{
  if (!wire.hasWire()) {
    throw Error("The supplied block does not contain wire format");
  }

  m_hasKeyLocator = false;

  m_wire = wire;
  m_wire.parse();

  if (m_wire.type() != tlv::EncryptedContent)
    throw Error("Unexpected TLV type when decoding Name");

  Block::element_const_iterator it = m_wire.elements_begin();

  if (it != m_wire.elements_end() && it->type() == ndn::tlv::KeyLocator) {
    m_keyLocator.wireDecode(*it);
    m_hasKeyLocator = true;
    it++;
  }
  else
    throw Error("EncryptedContent does not have key locator");

  if (it != m_wire.elements_end() && it->type() == tlv::EncryptionAlgorithm) {
    m_type = readNonNegativeInteger(*it);
    it++;
  }
  else
    throw Error("EncryptedContent does not have encryption algorithm");

  if (it != m_wire.elements_end() && it->type() == tlv::EncryptedPayload) {
    m_payload = make_shared<Buffer>(it->value_begin(),it->value_end());
    it++;
  }
  else
    throw Error("EncryptedContent has missing payload");

  if (it != m_wire.elements_end()) {
    throw Error("EncryptedContent has extraneous sub-TLVs");
  }
}

bool
EncryptedContent::operator==(const EncryptedContent& rhs) const
{
  return (wireEncode() == rhs.wireEncode());
}

} // namespace gep
} // namespace ndn
