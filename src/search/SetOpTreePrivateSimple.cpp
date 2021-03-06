#include <sserialize/search/SetOpTreePrivateSimple.h>
#include <iostream>
#include <sserialize/vendor/utf8.h>

namespace sserialize {

StringCompleter::QuerryType qtfghFromString(std::string & str) {
	char t = 1;
	if (str.size() == 0) {
		return StringCompleter::QT_PREFIX;
	}
	if (*(str.begin()) == '\"') {
		str.erase(0,1);
		if (str.size() > 0 && *(str.rbegin()) == '\"') {
			str.erase(str.size()-1, 1);
		}
	}
	else {
		if (*(str.begin()) == '*') {
			t = t << 2;
			str.erase(0, 1);
		}
		if (str.size() >  0 && *(str.rbegin()) == '*') {
			t = t << 1;
			str.erase(str.size()-1, 1);
		}
		if (t == 1) //default to QT_SUFFIX_PREFIX if no " are given
			t = 8;
	}
	switch (t) {
	case (1):
		return StringCompleter::QT_EXACT;
	case (2):
		return StringCompleter::QT_PREFIX;
	case (4):
		return StringCompleter::QT_SUFFIX;
	case (8):
	default:
		return StringCompleter::QT_SUBSTRING;
	}
}


void SetOpTreePrivateSimple::handParse(const std::string & qstr) {
	if (qstr.size() == 0)
		return;
	QueryStringParserStates state = QSPS_START;
	std::string::const_iterator it = qstr.begin();
	std::string::const_iterator end = qstr.end();
	try { //fetch invalid utf8 encoding errors on the fly
	
		bool curIsDiff = false;
		QueryStringDescription qsd;
		std::back_insert_iterator<std::string> qsdBIt(qsd.str());
		
		do {
			uint32_t ucode = utf8::next(it, end);
			switch (state) {
			case QSPS_START:
				if (ucode == '-') {
					curIsDiff = true;
					break;
				}
				else if (ucode == ' ') {
					curIsDiff = false;
					break;
				}
				else {
					state = QSPS_READ_TOKEN;
					//fall through to QSPS_READ_TOKEN to copy current ucode
					[[fallthrough]];
				}
			case QSPS_READ_TOKEN:
				if (ucode == '\\') {
					state = QSPS_CP_NEXT;
				}
				else if (ucode == ' ') {
					//set the qt-flags
					qsd.qt() = qtfghFromString(qsd.str());
					
					if (qsd.str().size() >= m_minStrLen) {
						if(curIsDiff) {
							m_diffStrings.push_back(qsd);
						}
						else {
							m_intersectStrings.push_back(qsd);
						}
					}
					curIsDiff = false;
					qsd = QueryStringDescription();
					qsdBIt = std::back_insert_iterator<std::string>(qsd.str());
					state = QSPS_START;
				}
				else {
					utf8::append(ucode, qsdBIt);
				}
				break;
			case QSPS_CP_NEXT:
				utf8::append(ucode, qsdBIt);
				state = QSPS_READ_TOKEN;
				break;
			default:
				std::cerr << "sserialize::SetOpTreePrivateSimple::buildTree:: undefined parser state reached" << std::endl;
			}
		} while (it != end);
		
		if (qsd.str().size()) {
			qsd.qt() = qtfghFromString(qsd.str());
			
			if (qsd.str().size() >= m_minStrLen) {
				if(curIsDiff) {
					m_diffStrings.push_back(qsd);
				}
				else {
					m_intersectStrings.push_back(qsd);
				}
			}
		}
	}
	catch (utf8::exception & e) {
		std::cerr << "Caught exception in SetOpTreePrivateSimple::builTree: " << e.what() << std::endl;
	}
}

void SetOpTreePrivateSimple::buildTree(const std::string & qstr) {
	m_diffStrings.clear();
	m_intersectStrings.clear();

	if (m_ef.size())
		ragelParse(qstr);
	else
		handParse(qstr);
}

ItemIndexIterator SetOpTreePrivateSimple::asItemIndexIterator() {
	if (m_intersectStrings.size()) {
		std::vector<ItemIndexIterator> intersectIts, diffIts;
		for(std::vector<QueryStringDescription>::iterator it = m_intersectStrings.begin(); it != m_intersectStrings.end(); ++it) {
			ItemIndexIterator intersectIt;
			if (it->ef().priv()) {
				intersectIt = it->ef()->partialComplete(it->str());
			}
			else {
				intersectIt = m_strCompleter.partialComplete(it->str(), (sserialize::StringCompleter::QuerryType) it->qt());
			}
			if (intersectIt.maxSize() > 0)
				intersectIts.push_back(intersectIt);
		}
		
		for(std::vector<QueryStringDescription>::iterator it = m_diffStrings.begin(); it != m_diffStrings.end(); ++it) {
			ItemIndexIterator diffIt;
			if (it->ef().priv()) {
				diffIt = it->ef()->partialComplete(it->str());
			}
			else {
				diffIt = m_strCompleter.partialComplete(it->str(), (sserialize::StringCompleter::QuerryType) it->qt());
			}

			if (diffIt.maxSize() > 0)
				diffIts.push_back(diffIt);
		}
		
		
		if (diffIts.size()) {
			return 
				ItemIndexIterator::createTree(intersectIts.begin(), intersectIts.end(), &sserialize::createIntersectItemIndexIterator) -
				ItemIndexIterator::createTree(diffIts.begin(), diffIts.end(), &sserialize::createIntersectItemIndexIterator);
		}
		else {
			ItemIndexIterator::createTree(intersectIts.begin(), intersectIts.end(), &sserialize::createIntersectItemIndexIterator);
		}
	}
	return ItemIndexIterator();
}


SetOpTreePrivate * SetOpTreePrivateSimple::copy() const {
	return new SetOpTreePrivateSimple(*this);
}

void SetOpTreePrivateSimple::doCompletions() {
	for(std::vector<QueryStringDescription>::iterator it = m_intersectStrings.begin(); it != m_intersectStrings.end(); ++it) {
		if (m_completions.count(*it) == 0) {
			if (it->qt() == sserialize::StringCompleter::QT_NONE) {
				m_completions[*it]  = it->ef()->complete(it->str());
			}
			else {
				m_completions[*it] = m_strCompleter.complete(it->str(), (sserialize::StringCompleter::QuerryType) it->qt());
			}
		}
	}
	
	for(std::vector<QueryStringDescription>::iterator it = m_diffStrings.begin(); it != m_diffStrings.end(); ++it) {
		if (m_completions.count(*it) == 0) {
			if (it->qt() == sserialize::StringCompleter::QT_NONE) {
				m_completions[*it]  = it->ef()->complete(it->str());
			}
			else {
				m_completions[*it] = m_strCompleter.complete(it->str(), (sserialize::StringCompleter::QuerryType) it->qt());
			}
		}
	}
}


ItemIndex SetOpTreePrivateSimple::doSetOperations() {
	if (m_intersectStrings.size()) {
		std::vector<ItemIndex> intersectIdx, diffIdx;
		for(std::vector<QueryStringDescription>::const_iterator it = m_intersectStrings.begin(); it != m_intersectStrings.end(); ++it) {
			ItemIndex idx = m_completions.at(*it);
			if (idx.size())
				intersectIdx.push_back( idx );
			else
				return ItemIndex();
		}
		
		for(std::vector<QueryStringDescription>::const_iterator it = m_diffStrings.begin(); it != m_diffStrings.end(); ++it) {
			ItemIndex idx = m_completions.at(*it);
			if (idx.size())
				diffIdx.push_back( idx );
		}
		
		if (intersectIdx.size() > 0) {
			if (diffIdx.size())
				return ItemIndex::fusedIntersectDifference(intersectIdx, diffIdx, m_maxResultSetSize);
			else
				return ItemIndex::constrainedIntersect(intersectIdx, m_maxResultSetSize);
		}
	}
	return ItemIndex();
}

bool SetOpTreePrivateSimple::registerStringCompleter(const sserialize::StringCompleter & stringCompleter) {
	m_strCompleter = stringCompleter;
	return true;
}

bool SetOpTreePrivateSimple::registerExternalFunction(SetOpTree::ExternalFunctoid * function) {
	if (!function)
		return false;
	m_ef[function->cmdString()] = RCPtrWrapper<SetOpTree::SelectableOpFilter>(function);
	return true;
}

bool SetOpTreePrivateSimple::registerSelectableOpFilter(SetOpTree::SelectableOpFilter * filter) {
	if (!filter)
		return false;
	m_ef[filter->cmdString()] = RCPtrWrapper<SetOpTree::SelectableOpFilter>(filter);
	return true;
}

SetOpTreePrivateSimple::SetOpTreePrivateSimple(const SetOpTreePrivateSimple & other) : 
	SetOpTreePrivate(),
	m_strCompleter(other.m_strCompleter),
	m_intersectStrings(other.m_intersectStrings),
	m_diffStrings(other.m_diffStrings),
	m_completions(other.m_completions),
	m_ef(other.m_ef),
	m_maxResultSetSize(other.m_maxResultSetSize),
	m_minStrLen(other.m_minStrLen)
{}

SetOpTreePrivateSimple::SetOpTreePrivateSimple() {}

SetOpTreePrivateSimple::~SetOpTreePrivateSimple() {}

ItemIndex SetOpTreePrivateSimple::update(const std::string & queryString) {
// 	std::cout << "SetOpTreePrivateSimple::update(" << queryString << ") with up to " << m_maxResultSetSize << std::endl;
	buildTree(queryString); //clears 
	
	std::map< QueryStringDescription, ItemIndex> tmp;
	
	for(std::vector<QueryStringDescription>::iterator it = m_intersectStrings.begin(); it != m_intersectStrings.end(); ++it) {
		if (m_completions.count(*it) == 0) {
			if (it->qt() == sserialize::StringCompleter::QT_NONE) {
				tmp[*it]  = it->ef()->complete(it->str());
			}
			else {
				tmp[*it] = m_strCompleter.complete(it->str(), (sserialize::StringCompleter::QuerryType) it->qt());
			}
		}
		else {
			tmp[*it] = m_completions[*it];
		}
	}
	
	for(std::vector<QueryStringDescription>::iterator it = m_diffStrings.begin(); it != m_diffStrings.end(); ++it) {
		if (m_completions.count(*it) == 0) {
			if (it->qt() == sserialize::StringCompleter::QT_NONE) {
				tmp[*it]  = it->ef()->complete(it->str());
			}
			else {
				tmp[*it] = m_strCompleter.complete(it->str(), (sserialize::StringCompleter::QuerryType) it->qt());
			}
		}
		else {
			tmp[*it] = m_completions[*it];
		}
	}
	m_completions.swap(tmp);
	return doSetOperations();
}

void SetOpTreePrivateSimple::clear() {
	m_completions.clear();
	m_diffStrings.clear();
	m_intersectStrings.clear();
}

std::ostream & SetOpTreePrivateSimple::printStructure(std::ostream & out) const {
	for(std::vector<QueryStringDescription>::const_iterator it = m_intersectStrings.begin(); it != m_intersectStrings.end();) {
		if (it->ef().priv()) {
			out << "$" << it->ef()->cmdString() << "[" << it->str() << "]";
		}
		else {
			if (it->qt() & (sserialize::StringCompleter::QT_SUFFIX | sserialize::StringCompleter::QT_SUBSTRING)) {
				out << "*";
			}
			else if (it->qt() & sserialize::StringCompleter::QT_EXACT) {
				out << "\"";
			}
			out << it->str();
			if (it->qt() & (sserialize::StringCompleter::QT_PREFIX | sserialize::StringCompleter::QT_SUBSTRING)) {
				out << "*";
			}
			else if (it->qt() & sserialize::StringCompleter::QT_EXACT) {
				out << "\"";
			}
		}
		++it;
		if (it != m_intersectStrings.end())
			out << " ";
	}
	
	if (m_intersectStrings.size() && m_diffStrings.size())
		out << " ";
	
	for(std::vector<QueryStringDescription>::const_iterator it = m_diffStrings.begin(); it != m_diffStrings.end();) {
		out << "-";
		if (it->ef().priv()) {
			out << "$" << it->ef()->cmdString() << "[" << it->str() << "]";
		}
		else {
			if (it->qt() & (sserialize::StringCompleter::QT_SUFFIX | sserialize::StringCompleter::QT_SUBSTRING)) {
				out << "*";
			}
			else if (it->qt() & sserialize::StringCompleter::QT_EXACT) {
				out << "\"";
			}
			
			out << it->str();
			if (it->qt() & (sserialize::StringCompleter::QT_PREFIX | sserialize::StringCompleter::QT_SUBSTRING)) {
				out << "*";
			}
			else if (it->qt() & sserialize::StringCompleter::QT_EXACT) {
				out << "\"";
			}
		}
		++it;
		if (it != m_diffStrings.end())
			out << " ";
	}
	return out;
}




}//end namespace
