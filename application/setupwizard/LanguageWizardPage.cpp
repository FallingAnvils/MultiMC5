#include "LanguageWizardPage.h"
#include <MultiServerMC.h>
#include <translations/TranslationsModel.h>

#include "widgets/LanguageSelectionWidget.h"
#include <QVBoxLayout>

LanguageWizardPage::LanguageWizardPage(QWidget *parent)
    : BaseWizardPage(parent)
{
    setObjectName(QStringLiteral("languagePage"));
    auto layout = new QVBoxLayout(this);
    mainWidget = new LanguageSelectionWidget(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(mainWidget);

    retranslate();
}

LanguageWizardPage::~LanguageWizardPage()
{
}

bool LanguageWizardPage::wantsRefreshButton()
{
    return true;
}

void LanguageWizardPage::refresh()
{
    auto translations = MSMC->translations();
    translations->downloadIndex();
}

bool LanguageWizardPage::validatePage()
{
    auto settings = MSMC->settings();
    QString key = mainWidget->getSelectedLanguageKey();
    settings->set("Language", key);
    return true;
}

void LanguageWizardPage::retranslate()
{
    setTitle(tr("Language"));
    setSubTitle(tr("Select the language to use in MultiServerMC"));
    mainWidget->retranslate();
}
