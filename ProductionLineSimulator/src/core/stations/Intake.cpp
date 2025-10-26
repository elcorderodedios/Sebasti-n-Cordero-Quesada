#include "Intake.h"
#include <QRandomGenerator>
#include <QThread>

Intake::Intake(QObject* parent)
    : WorkStation("Intake", parent)
    , m_productionRate(10) // 10 products per minute default
    , m_productionTimer(new QTimer(this))
{
    setProcessingTime(50, 150); // Quick intake processing
    
    // Setup production timer
    connect(m_productionTimer, &QTimer::timeout, this, &Intake::onProductionTimer);
    m_productionTimer->setInterval(60000 / m_productionRate); // Convert to milliseconds
}

bool Intake::processProduct(std::shared_ptr<Product> product)
{
    // Simulate intake processing
    QThread::msleep(getRandomProcessingTime());
    
    // Set product state
    product->setState(ProductState::AtIntake);
    product->addTraceEntry("Intake");
    product->advanceState(); // Move to next state
    
    logActivity(QString("Processed product %1 (%2)").arg(product->getId(), product->getTypeString()));
    
    return true; // Intake rarely fails
}

void Intake::onStationStarted()
{
    m_productionTimer->start();
    logActivity("Started product generation");
}

void Intake::onStationStopped()
{
    m_productionTimer->stop();
    logActivity("Stopped product generation");
}

void Intake::generateNewProduct()
{
    // Create a new product with random type
    auto types = {ProductType::Washer, ProductType::Dryer, ProductType::Refrigerator, 
                  ProductType::Dishwasher, ProductType::Oven};
    auto randomType = *(types.begin() + QRandomGenerator::global()->bounded(types.size()));
    
    auto product = std::make_shared<Product>(randomType);
    
    // Try to add to output buffer (non-blocking)
    if (getOutputBuffer() && getOutputBuffer()->tryPush(product)) {
        logActivity(QString("Generated new product %1 (%2)").arg(product->getId(), product->getTypeString()));
    } else {
        logActivity("Output buffer full, product generation skipped");
    }
}

void Intake::onProductionTimer()
{
    if (getState() == StationState::Running) {
        generateNewProduct();
    }
}
