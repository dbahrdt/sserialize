#pragma once

#include <sserialize/spatial/dgg/HCQRIndex.h>

namespace sserialize::spatial::dgg {

class HCQRIndexMakeStatic: public sserialize::spatial::dgg::interface::HCQRIndex {
public:
    using HCQRIndexPtr = sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::HCQRIndex>;
public:
    HCQRIndexMakeStatic(HCQRIndexPtr const & base);
    ~HCQRIndexMakeStatic() override;
	static HCQRIndexPtr make(HCQRIndexPtr const & base);
public:
    void setCacheSize(uint32_t size);
public:
    sserialize::StringCompleter::SupportedQuerries getSupportedQueries() const override;
public:
	HCQRPtr complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
	HCQRPtr items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
	HCQRPtr regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
public:
	HCQRPtr cell(uint32_t cellId) const override;
	HCQRPtr region(uint32_t regionId) const override;
public:
	SpatialGridInfo const & sgi() const override;
	SpatialGrid const & sg() const override;
private:
    HCQRIndexPtr m_base;
};

}//end namespace sserialize::spatial::dgg
