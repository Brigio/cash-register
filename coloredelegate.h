#ifndef COLOREDELEGATE_H
#define COLOREDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QLocale>

class ColoreDelegate : public QStyledItemDelegate
{
public:
    explicit ColoreDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent),
        itLocale(QLocale(QLocale::Italian, QLocale::Italy))
    {}

    QString displayText(const QVariant &value, const QLocale &locale) const override
    {
        bool ok = false;
        double num = value.toDouble(&ok);

        if (ok)
            return itLocale.toCurrencyString(num, "€");

        return QStyledItemDelegate::displayText(value, locale);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);

        // Allineamento a destra per numeri
        opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;

        bool ok = false;
        double num = index.data(Qt::DisplayRole).toDouble(&ok);

        if (ok)
        {
            // Se la cella NON è selezionata → applico colore
            if (!(option.state & QStyle::State_Selected)) {
                if (num < 0)
                    opt.palette.setColor(QPalette::Text, Qt::red);
                else
                    opt.palette.setColor(QPalette::Text, QColor("#004000")); // verde scuro
            }
        }

        QStyledItemDelegate::paint(painter, opt, index);
    }

private:
    QLocale itLocale;
};

#endif // COLOREDELEGATE_H
