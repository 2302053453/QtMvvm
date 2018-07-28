#include "settingsconfigimpl.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QFileSelector>

namespace {

using SourceType = SettingsConfigBase::variant<SettingsConfigBase::SettingsConfigType, SettingsConfigBase::CategoryType, SettingsConfigBase::SectionType, SettingsConfigBase::GroupType, SettingsConfigBase::EntryType>;

template <typename TVariantTarget>
void convertElement(QXmlStreamReader &reader, TVariantTarget &, SourceType &&)
{
	throw SettingsConfigBase::XmlException{reader, QStringLiteral("Unexpected root element in included file")};
}

template <typename TVariantTarget, typename TType, typename... TArgs>
void convertElement(QXmlStreamReader &reader, TVariantTarget &target, SourceType &&source)
{
	if(nonstd::holds_alternative<TType>(source))
		target = nonstd::get<TType>(std::move(source));
	else
		convertElement<TVariantTarget, TArgs...>(reader, target, std::move(source));
}

template <typename T>
struct VariantInfo {};

template <typename... TArgs>
struct VariantInfo<QList<SettingsConfigBase::variant<SettingsConfigBase::IncludeType, TArgs...>>>
{
	using TargetType = SettingsConfigBase::variant<SettingsConfigBase::IncludeType, TArgs...>;
	static void convert(QXmlStreamReader &reader, TargetType &target, SourceType &&source) {
		convertElement<TargetType, TArgs...>(reader, target, std::move(source));
	}
};

}

void SettingsConfigImpl::setFilters(QString frontend, const QFileSelector *selector)
{
	_frontend = std::move(frontend);
	_selector = selector;
}

void SettingsConfigImpl::resetFilters()
{
	_frontend.clear();
	_selector = nullptr;
}

void SettingsConfigImpl::setIncludesOptional(bool alwaysOptional)
{
	_alwaysOptional = alwaysOptional;
}

bool SettingsConfigImpl::finish_group_content(QXmlStreamReader &reader, GroupContentGroup &data, bool hasNext)
{
	hasNext = read_GroupContentGroup(reader, data, hasNext);
	finishContents(reader, data.content);
	return hasNext;
}

bool SettingsConfigImpl::finish_section_content(QXmlStreamReader &reader, SectionContentGroup &data, bool hasNext)
{
	hasNext = read_SectionContentGroup(reader, data, hasNext);
	finishContents(reader, data.content);
	return hasNext;
}

bool SettingsConfigImpl::finish_category_content(QXmlStreamReader &reader, CategoryContentGroup &data, bool hasNext)
{
	hasNext = read_CategoryContentGroup(reader, data, hasNext);
	finishContents(reader, data.content);
	return hasNext;
}

bool SettingsConfigImpl::finish_settings_config_content(QXmlStreamReader &reader, SettingsConfigContentGroup &data, bool hasNext)
{
	hasNext = read_SettingsConfigContentGroup(reader, data, hasNext);
	finishContents(reader, data.content);
	return hasNext;
}

template<typename TGroup>
void SettingsConfigImpl::finishContents(QXmlStreamReader &reader, TGroup &group)
{
	optional<std::size_t> index;
	for(auto it = group.begin(); it != group.end();) {
		// convert includes to the actual data
		if(nonstd::holds_alternative<IncludeType>(*it)) {
			if(!isUsable(nonstd::get<IncludeType>(*it))) {
				it = group.erase(it);
				continue;
			}
			if(!readGeneralInclude(reader, nonstd::get<IncludeType>(*it), it, group))
				continue;
		}

		// verify that the contents are all of the same type
		if(index) {
			if(index.value() != it->index())
				throw XmlException{reader, QStringLiteral("Detected mixture of different child elements. Only includes and a single other type are allowed")};
		} else
			index = it->index();

		//erase if not usable due to selectors
		bool visited = false;
		nonstd::visit([&](auto &&element){ //TODO use trick if neccessary: https://en.cppreference.com/w/cpp/utility/variant/visit
			visited = true;
			if(isUsable(element))
				++it;
			else
				it = group.erase(it);
		}, *it);
		Q_ASSERT(visited);
	}
}

template<typename TIter, typename TList>
bool SettingsConfigImpl::readGeneralInclude(QXmlStreamReader &reader, IncludeType include, TIter &it, TList &list)
{
	try {
		if(_selector)
			include.includePath = _selector->select(include.includePath);

		//make the path relative if possbile
		if(dynamic_cast<QFileDevice*>(reader.device())) {
			QFileInfo docInfo{static_cast<QFileDevice*>(reader.device())->fileName()};
			include.includePath = docInfo.dir().absoluteFilePath(include.includePath);
		}
		// read the document
		VariantInfo<TList>::convert(reader, *it, readDocument(include.includePath));
		return true;
	} catch(FileException &e) {
		if(include.optional || _alwaysOptional) {
			qWarning() << e.what();
			it = list.erase(it);
			return false;
		} else
			throw;
	}
}

bool SettingsConfigImpl::isUsable(const SelectableContrainerInfo &element) const
{
	if(!_frontend.isNull() && element.frontends) {
		auto fronts = element.frontends.value().split(QLatin1Char('|'), QString::SkipEmptyParts);
		if(!fronts.contains(_frontend))
			return false;
	}

	if(_selector && element.selectors) {
		auto selectors = _selector->allSelectors();
		auto selects = element.selectors.value().split(QLatin1Char('|'), QString::SkipEmptyParts);
		for(const auto &select : selects) {
			auto sels = select.split(QLatin1Char('&'), QString::SkipEmptyParts);
			auto allSelected = true;
			for(const auto &sel : sels) {
				if(!selectors.contains(sel)) {
					allSelected = false;
					break;
				}
			}
			if(allSelected)
				return true;
		}
		return false;
	}

	return true;
}

