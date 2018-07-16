#ifndef TESTBACKEND_H
#define TESTBACKEND_H

#include <QtMvvmCore/ISettingsAccessor>

class TestBackend : public QtMvvm::ISettingsAccessor
{
	Q_OBJECT

public:
	explicit TestBackend(QString name, int code, QObject *parent = nullptr);

	// ISettingsAccessor interface
public:
	bool contains(const QString &key) const override;
	QVariant load(const QString &key, const QVariant &defaultValue) const override;
	void save(const QString &key, const QVariant &value) override;
	void remove(const QString &key) override;

public slots:
	void sync() override;

public:
	QString _name;
	int _code;
};

#define SOME_EXPORT

#endif // TESTBACKEND_H
