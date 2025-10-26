#ifndef PRODUCT_H
#define PRODUCT_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QStringList>
#include <QUuid>

enum class ProductType {
    Washer,
    Dryer,
    Refrigerator,
    Dishwasher,
    Oven
};

enum class ProductState {
    Created,
    AtIntake,
    AtAssembler,
    AtQualityInspection,
    AtPackaging,
    AtShipping,
    Finished,
    Rejected,
    InRework
};

class Product
{
public:
    explicit Product(ProductType type = ProductType::Washer);
    
    // Getters
    QString getId() const { return m_id; }
    ProductType getType() const { return m_type; }
    ProductState getCurrentState() const { return m_currentState; }
    QDateTime getCreatedTime() const { return m_createdTime; }
    QStringList getTrace() const { return m_trace; }
    
    // State management
    void advanceState();
    void setState(ProductState state);
    void addTraceEntry(const QString& station);
    void setReworkFlag(bool rework) { m_inRework = rework; }
    bool isInRework() const { return m_inRework; }
    
    // Information
    QString showInfo() const;
    QString getTypeString() const;
    QString getStateString() const;
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

private:
    QString m_id;
    ProductType m_type;
    ProductState m_currentState;
    QDateTime m_createdTime;
    QStringList m_trace;
    bool m_inRework;
    
    QString generateId() const;
};

#endif // PRODUCT_H
