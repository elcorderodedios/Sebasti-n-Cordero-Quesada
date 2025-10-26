#ifndef SHIPPING_H
#define SHIPPING_H

#include "../WorkStation.h"

class Shipping : public WorkStation
{
    Q_OBJECT

public:
    explicit Shipping(QObject* parent = nullptr);

protected:
    bool processProduct(std::shared_ptr<Product> product) override;

private:
    struct ShippingInfo {
        QString destination;
        QString method;
        QString trackingNumber;
        QDateTime estimatedDelivery;
    };
    
    ShippingInfo generateShippingInfo(std::shared_ptr<Product> product);
    QString generateTrackingNumber() const;
    QDateTime calculateDeliveryDate(const QString& method) const;
    
    QStringList m_destinations;
    QStringList m_shippingMethods;
};

#endif // SHIPPING_H
