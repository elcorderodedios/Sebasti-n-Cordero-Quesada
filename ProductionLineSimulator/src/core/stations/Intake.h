#ifndef INTAKE_H
#define INTAKE_H

#include "../WorkStation.h"
#include <QTimer>

class Intake : public WorkStation
{
    Q_OBJECT

public:
    explicit Intake(QObject* parent = nullptr);

protected:
    bool processProduct(std::shared_ptr<Product> product) override;
    void onStationStarted() override;
    void onStationStopped() override;

private:
    void generateNewProduct();
    
    int m_productionRate; // products per minute
    QTimer* m_productionTimer;

private slots:
    void onProductionTimer();
};

#endif // INTAKE_H
