/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "projectcompilerwidget.h"
#include "ui_projectcompilerwidget.h"
#include "../settings.h"
#include "../project.h"
#include "../mainwindow.h"

ProjectCompilerWidget::ProjectCompilerWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectCompilerWidget)
{
    ui->setupUi(this);
}

ProjectCompilerWidget::~ProjectCompilerWidget()
{
    delete ui;
}

void ProjectCompilerWidget::refreshOptions()
{
    Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(ui->cbCompilerSet->currentIndex());
    if (!pSet)
        return;
    ui->chkAddCharset->setVisible(pSet->compilerType()!=COMPILER_CLANG);
    ui->chkAddCharset->setEnabled(pSet->compilerType()!=COMPILER_CLANG);
    mOptions = pMainWindow->project().options().compilerOptions;
    if (mOptions.isEmpty())
        mOptions = pSet->compileOptions();
    QTabWidget* pTab = ui->tabOptions;
    while (pTab->count()>0) {
        QWidget* p=pTab->widget(0);
        if (p!=nullptr) {
            pTab->removeTab(0);
            p->setParent(nullptr);
            delete p;
        }
    }

    for (int index=0; index< pSet->options().count();index++){
        PCompilerOption pOption =pSet->options()[index];
        QWidget* pWidget = nullptr;
        for (int i=0;i<pTab->count();i++) {
            if (pOption->section == pTab->tabText(i)) {
                pWidget = pTab->widget(i);
                break;
            }
        }
        if (pWidget == nullptr) {
            pWidget = new QWidget();
            pTab->addTab(pWidget,pOption->section);
            pWidget->setLayout(new QGridLayout());
        }
        QGridLayout *pLayout = static_cast<QGridLayout*>(pWidget->layout());
        int row = pLayout->rowCount();
        pLayout->addWidget(new QLabel(pOption->name),row,0);
        QComboBox* pCombo = new QComboBox();
        if (pOption->choices.count()>0) {
            for (int i=0;i<pOption->choices.count();i++) {
                QString choice = pOption->choices[i];
                QStringList valueName=choice.split("=");
                if (valueName.length()<2) {
                    pCombo->addItem("");
                } else {
                    pCombo->addItem(valueName[0]);
                }
            }
        } else {
            pCombo->addItem(QObject::tr("No"));
            pCombo->addItem(QObject::tr("Yes"));
        }
        int value;
        if (index<mOptions.length()) {
            value = pSet->charToValue(mOptions[index]);
        } else {
            value = pOption->value;
        }
        pCombo->setCurrentIndex(value);
        pLayout->addWidget(pCombo,row,1);
    }
    for (int i=0;i<pTab->count();i++) {
        QWidget* pWidget = pTab->widget(i);
        QGridLayout *pLayout = static_cast<QGridLayout*>(pWidget->layout());
        int row = pLayout->rowCount();
        QSpacerItem* horizontalSpacer = new QSpacerItem(10, 100, QSizePolicy::Minimum, QSizePolicy::Expanding);
        pLayout->addItem(horizontalSpacer,row,0);
    }

    ui->chkStaticLink->setChecked(pSet->staticLink());
}

void ProjectCompilerWidget::doLoad()
{
    ui->chkAddCharset->setChecked(pMainWindow->project()->options().addCharset);
    ui->chkStaticLink->setChecked(pMainWindow->project()->options().staticLink);

    mOptions = pMainWindow->project()->options().compilerOptions;
    ui->cbCompilerSet->setCurrentIndex(pMainWindow->project()->options().compilerSet);
}

void ProjectCompilerWidget::doSave()
{
    Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(ui->cbCompilerSet->currentIndex());
    if (!pSet)
        return;
    //read values in the options widget
    QTabWidget* pTab = ui->tabOptions;
    mOptions.clear();
    for (int i=0;i<pTab->count();i++) {
        QString section = pTab->tabText(i);
        QWidget* pWidget = pTab->widget(i);
        QGridLayout* pLayout = static_cast<QGridLayout*>(pWidget->layout());
        if (pLayout != nullptr) {
            for (int j=1;j<pLayout->rowCount()-1;j++) {
                QString name = static_cast<QLabel *>(pLayout->itemAtPosition(j,0)->widget())->text();
                QComboBox* pCombo = static_cast<QComboBox *>(pLayout->itemAtPosition(j,1)->widget());
                for (int index=0;index<pSet->options().count();index++) {
                     PCompilerOption pOption = pSet->options()[index];
                    if (pOption->section == section && pOption->name == name) {
                        char val = pSet->valueToChar( pCombo->currentIndex());
                        mOptions.append(val);
                    }
                }
            }
        }
    }
    pMainWindow->project()->setCompilerSet(ui->cbCompilerSet->currentIndex());
    pMainWindow->project()->options().compilerOptions = mOptions;
    if (pSet->compilerType()!=COMPILER_CLANG)
        pMainWindow->project()->options().addCharset = ui->chkAddCharset->isChecked();
    pMainWindow->project()->options().staticLink = ui->chkStaticLink->isChecked();
    pMainWindow->project()->saveOptions();
}

void ProjectCompilerWidget::init()
{
    ui->cbCompilerSet->clear();
    for (size_t i=0;i<pSettings->compilerSets().size();i++) {
        ui->cbCompilerSet->addItem(pSettings->compilerSets().getSet(i)->name());
    }
    SettingsWidget::init();
}

void ProjectCompilerWidget::on_cbCompilerSet_currentIndexChanged(int)
{
    refreshOptions();
}

