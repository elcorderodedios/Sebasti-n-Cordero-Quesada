#include "Assembler.h"
#include <QThread>
#include <QRandomGenerator>

Assembler::Assembler(QObject* parent)
    : WorkStation("Assembler", parent)
{
    setProcessingTime(200, 400); // Longer assembly time
    setFailureRate(0.02); // 2% failure rate
}

bool Assembler::processProduct(std::shared_ptr<Product> product)
{
    // Check if product should fail
    if (shouldRejectProduct()) {
        logActivity(QString("Assembly failed for product %1").arg(product->getId()));
        product->setState(ProductState::Rejected);
        return false;
    }
    
    // Get assembly steps for this product type
    QStringList steps = getAssemblySteps(product->getType());
    
    // Perform each assembly step
    for (const QString& step : steps) {
        performAssemblyStep(product, step);
        QThread::msleep(getRandomProcessingTime() / steps.size()); // Distribute time across steps
    }
    
    // Update product state
    product->setState(ProductState::AtAssembler);
    product->addTraceEntry("Assembler");
    product->advanceState();
    
    logActivity(QString("Assembled product %1 with %2 steps").arg(product->getId()).arg(steps.size()));
    
    return true;
}

void Assembler::performAssemblyStep(std::shared_ptr<Product> product, const QString& step)
{
    logActivity(QString("Performing %1 for product %2").arg(step, product->getId()));
    
    // Simulate step processing time
    QThread::msleep(10 + QRandomGenerator::global()->bounded(50));
}

QStringList Assembler::getAssemblySteps(ProductType type) const
{
    switch (type) {
        case ProductType::Washer:
            return {"Install drum", "Connect motor", "Install control panel", "Add door seal"};
        case ProductType::Dryer:
            return {"Install heating element", "Connect ventilation", "Install control panel", "Add lint filter"};
        case ProductType::Refrigerator:
            return {"Install compressor", "Add insulation", "Install shelves", "Connect cooling system"};
        case ProductType::Dishwasher:
            return {"Install spray arms", "Connect pump", "Install control panel", "Add door seals"};
        case ProductType::Oven:
            return {"Install heating elements", "Add insulation", "Install control panel", "Connect gas/electric"};
    }
    return {"Generic assembly"};
}
