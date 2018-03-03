#include "identityeditviewmodel_p.h"
#include "datasyncviewmodel.h"
using namespace QtMvvm;
using namespace QtDataSync;

IdentityEditViewModel::IdentityEditViewModel(QObject *parent) :
	ViewModel(parent),
	_manager(nullptr)
{}

QVariantHash IdentityEditViewModel::showParams(AccountManager *manager)
{
	return {
		{QStringLiteral("manager"), QVariant::fromValue(manager)}
	};
}

QString IdentityEditViewModel::name() const
{
	return _nameBuffer;
}

QString IdentityEditViewModel::fingerPrint() const
{
	return _manager ?
				DataSyncViewModel::formatFingerPrint(_manager->deviceFingerprint()) :
				QString();
}

void IdentityEditViewModel::save()
{
	if(_manager)
		_manager->setDeviceName(_nameBuffer);
}

void IdentityEditViewModel::setName(const QString &name)
{
	if (_nameBuffer == name)
		return;

	_nameBuffer = name;
	emit nameChanged(_nameBuffer);
}

void IdentityEditViewModel::onInit(const QVariantHash &params)
{
	_manager = params.value(QStringLiteral("manager")).value<AccountManager*>();

	connect(_manager, &AccountManager::deviceFingerprintChanged,
			this, &IdentityEditViewModel::fingerPrintChanged);
	connect(_manager, &AccountManager::deviceNameChanged,
			this, &IdentityEditViewModel::setName);
	setName(_manager->deviceName());
	emit fingerPrintChanged();
}
