#include "Shipping.h"
#include <QThread>
#include <QRandomGenerator>
#include <QUuid>

Shipping::Shipping(QObject* parent)
    : WorkStation("Shipping", parent)
{
    setProcessingTime(100, 200); // Quick shipping processing
    setFailureRate(0.005); // 0.5% failure rate - very rare shipping issues
    
    // Initialize destinations and shipping methods
    m_destinations = {"New York", "Los Angeles", "Chicago", "Houston", "Phoenix", 
                     "Philadelphia", "San Antonio", "San Diego", "Dallas", "San Jose"};
    
    m_shippingMethods = {"Standard Ground", "Express", "Next Day Air", "Freight"};
}

bool Shipping::processProduct(std::shared_ptr<Product> product)
{
    // Very rare shipping failures (lost packages, damage during transit prep)
    if (shouldRejectProduct()) {
        logActivity(QString("Shipping preparation failed for product %1 - package damaged").arg(product->getId()));
        product->setState(ProductState::Rejected);
        return false;
    }
    
    // Generate shipping information
    ShippingInfo shippingInfo = generateShippingInfo(product);
    
    logActivity(QString("Processing shipment for product %1 to %2")
                .arg(product->getId(), shippingInfo.destination));
    
    // Simulate shipping preparation steps
    QThread::msleep(getRandomProcessingTime() / 4);
    logActivity(QString("Generated shipping label for product %1").arg(product->getId()));
    
    QThread::msleep(getRandomProcessingTime() / 4);
    logActivity(QString("Product %1 loaded for %2 shipping").arg(product->getId(), shippingInfo.method));
    
    QThread::msleep(getRandomProcessingTime() / 4);
    logActivity(QString("Tracking number %1 assigned to product %2")
                .arg(shippingInfo.trackingNumber, product->getId()));
    
    QThread::msleep(getRandomProcessingTime() / 4);
    logActivity(QString("Product %1 dispatched - ETA: %2")
                .arg(product->getId(), shippingInfo.estimatedDelivery.toString()));
    
    // Finalize product
    product->setState(ProductState::AtShipping);
    product->addTraceEntry(QString("Shipping - %1 to %2")
                          .arg(shippingInfo.method, shippingInfo.destination));
    product->advanceState(); // This should set it to Finished
    
    logActivity(QString("Product %1 successfully shipped to %2 via %3 (Tracking: %4)")
                .arg(product->getId(), shippingInfo.destination, 
                     shippingInfo.method, shippingInfo.trackingNumber));
    
    return true;
}

Shipping::ShippingInfo Shipping::generateShippingInfo(std::shared_ptr<Product> product)
{
    ShippingInfo info;
    
    // Random destination
    info.destination = m_destinations[QRandomGenerator::global()->bounded(m_destinations.size())];
    
    // Shipping method based on product type (heavier items use freight)
    if (product->getType() == ProductType::Refrigerator) {
        info.method = "Freight";
    } else {
        info.method = m_shippingMethods[QRandomGenerator::global()->bounded(m_shippingMethods.size())];
    }
    
    // Generate tracking number
    info.trackingNumber = generateTrackingNumber();
    
    // Calculate delivery date
    info.estimatedDelivery = calculateDeliveryDate(info.method);
    
    return info;
}

QString Shipping::generateTrackingNumber() const
{
    // Generate a realistic tracking number format
    QString prefix = QString("1Z%1").arg(QRandomGenerator::global()->bounded(100000, 999999));
    QString suffix = QUuid::createUuid().toString().mid(1, 8).toUpper().remove('-');
    return QString("%1%2").arg(prefix, suffix);
}

QDateTime Shipping::calculateDeliveryDate(const QString& method) const
{
    QDateTime now = QDateTime::currentDateTime();
    int daysToAdd = 0;
    
    if (method == "Next Day Air") {
        daysToAdd = 1;
    } else if (method == "Express") {
        daysToAdd = 2 + QRandomGenerator::global()->bounded(2); // 2-3 days
    } else if (method == "Standard Ground") {
        daysToAdd = 5 + QRandomGenerator::global()->bounded(3); // 5-7 days
    } else if (method == "Freight") {
        daysToAdd = 7 + QRandomGenerator::global()->bounded(7); // 7-14 days
    }
    
    return now.addDays(daysToAdd);
}
