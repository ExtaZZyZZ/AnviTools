#include "MonitorTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QClipboard>
#include <QApplication>
#include <QTimer>

MonitorTab::MonitorTab(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // ── Toolbar row ───────────────────────────────────────────────────────────
    auto* toolbar = new QHBoxLayout();
    clearBtn_      = new QPushButton("Очистить", this);
    clearBtn_->setFixedWidth(100);
    copyAllBtn_    = new QPushButton("Копировать всё", this);
    autoScrollChk_ = new QCheckBox("Автопрокрутка", this);
    autoScrollChk_->setChecked(true);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(copyAllBtn_);
    toolbar->addWidget(autoScrollChk_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    // ── Search row ────────────────────────────────────────────────────────────
    auto* searchRow   = new QHBoxLayout();
    auto* searchLabel = new QLabel("Поиск:", this);
    searchEdit_       = new QLineEdit(this);
    searchEdit_->setPlaceholderText("Введите текст для поиска...");
    searchEdit_->setClearButtonEnabled(true);
    findPrevBtn_     = new QToolButton(this);
    findNextBtn_     = new QToolButton(this);
    findPrevBtn_->setArrowType(Qt::UpArrow);
    findNextBtn_->setArrowType(Qt::DownArrow);
    matchCountLabel_ = new QLabel("", this);
    matchCountLabel_->setMinimumWidth(80);
    matchCountLabel_->setStyleSheet("color: #888888;");
    findPrevBtn_->setFixedWidth(28);
    findNextBtn_->setFixedWidth(28);
    findPrevBtn_->setToolTip("Найти предыдущее");
    findNextBtn_->setToolTip("Найти следующее (Enter)");

    searchRow->addWidget(searchLabel);
    searchRow->addWidget(searchEdit_, 1);
    searchRow->addWidget(findPrevBtn_);
    searchRow->addWidget(findNextBtn_);
    searchRow->addWidget(matchCountLabel_);
    layout->addLayout(searchRow);

    // ── Text area ─────────────────────────────────────────────────────────────
    textEdit_ = new QTextEdit(this);
    textEdit_->setReadOnly(true);
    textEdit_->setFont(QFont("Courier New", 9));
    textEdit_->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(textEdit_);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(clearBtn_,    &QPushButton::clicked,    this, &MonitorTab::onClear);
    connect(copyAllBtn_,  &QPushButton::clicked,    this, &MonitorTab::onCopyAll);
    connect(searchEdit_,  &QLineEdit::textChanged,  this, &MonitorTab::onSearchChanged);
    connect(searchEdit_,  &QLineEdit::returnPressed,this, &MonitorTab::onFindNext);
    connect(findNextBtn_, &QToolButton::clicked,    this, &MonitorTab::onFindNext);
    connect(findPrevBtn_, &QToolButton::clicked,    this, &MonitorTab::onFindPrev);
}

void MonitorTab::appendLine(const QString& line)
{
    QTextCharFormat fmt;
    fmt.setForeground(colorForLine(line));

    QTextCursor cursor(textEdit_->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(line + "\n", fmt);

    if (autoScrollChk_->isChecked())
        textEdit_->verticalScrollBar()->setValue(
            textEdit_->verticalScrollBar()->maximum());

    const QString term = searchEdit_->text();
    if (!term.isEmpty())
        highlightAllMatches(term);
}

void MonitorTab::onClear()
{
    textEdit_->clear();
    matchCountLabel_->setText("");
}

void MonitorTab::onCopyAll()
{
    QApplication::clipboard()->setText(textEdit_->toPlainText());
    copyAllBtn_->setText("Скопировано!");
    QTimer::singleShot(1500, this, [this]() {
        copyAllBtn_->setText("Копировать всё");
    });
}

void MonitorTab::onSearchChanged(const QString& term)
{
    if (term.isEmpty()) {
        textEdit_->setExtraSelections({});
        matchCountLabel_->setText("");
        matchCountLabel_->setStyleSheet("color: #888888;");
        return;
    }
    highlightAllMatches(term);

    QTextCursor c = textEdit_->document()->find(term, 0);
    if (!c.isNull())
        textEdit_->setTextCursor(c);
}

void MonitorTab::onFindNext()
{
    const QString term = searchEdit_->text();
    if (term.isEmpty()) return;

    if (!textEdit_->find(term)) {
        textEdit_->moveCursor(QTextCursor::Start);
        textEdit_->find(term);
    }
}

void MonitorTab::onFindPrev()
{
    const QString term = searchEdit_->text();
    if (term.isEmpty()) return;

    if (!textEdit_->find(term, QTextDocument::FindBackward)) {
        textEdit_->moveCursor(QTextCursor::End);
        textEdit_->find(term, QTextDocument::FindBackward);
    }
}

void MonitorTab::highlightAllMatches(const QString& term)
{
    QList<QTextEdit::ExtraSelection> selections;

    QTextDocument* doc = textEdit_->document();
    QTextCursor c(doc);

    QTextCharFormat hlFmt;
    hlFmt.setBackground(QColor("#4a3f00"));
    hlFmt.setForeground(QColor("#ffff00"));

    while (true) {
        c = doc->find(term, c);
        if (c.isNull()) break;
        QTextEdit::ExtraSelection sel;
        sel.cursor = c;
        sel.format = hlFmt;
        selections.append(sel);
    }

    textEdit_->setExtraSelections(selections);

    const int n = selections.size();
    if (n == 0) {
        matchCountLabel_->setText("Не найдено");
        matchCountLabel_->setStyleSheet("color: #f44747;");
    } else {
        matchCountLabel_->setText(QString("%1 совп.").arg(n));
        matchCountLabel_->setStyleSheet("color: #6adb6a;");
    }
}

int MonitorTab::countMatches(const QString& term) const
{
    if (term.isEmpty()) return 0;
    int count = 0;
    QTextCursor c(textEdit_->document());
    while (true) {
        c = textEdit_->document()->find(term, c);
        if (c.isNull()) break;
        ++count;
    }
    return count;
}

QColor MonitorTab::colorForLine(const QString& line) const
{
    if (line.contains("[OK]"))    return QColor("#6adb6a");
    if (line.contains("[ERR]"))   return QColor("#f44747");
    if (line.contains("[WARN]"))  return QColor("#ffcc00");
    if (line.contains("[SLEEP]")) return QColor("#569cd6");
    if (line.contains("[RTC]"))   return QColor("#4ec9b0");
    if (line.contains("[SPI]"))   return QColor("#ce9178");
    if (line.contains("[INFO]"))  return QColor("#9cdcfe");
    if (line.contains("***"))     return QColor("#c586c0");
    if (line.contains("[BOOT]"))  return QColor("#dcdcaa");
    return QColor("#d4d4d4");
}
