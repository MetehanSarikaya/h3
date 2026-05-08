#include "mainwindow.h"

#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Assignment 3 Spring 36");
    resize(420, 220);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *titleLabel = new QLabel("Qt Template App", this);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *sampleButton = new QPushButton("Show Sample Message", this);

    layout->addWidget(titleLabel);
    layout->addWidget(sampleButton);
    layout->addStretch();

    connect(sampleButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(
            this,
            "Sample Message",
            "The sample button was clicked."
        );
    });
}
