#include "Packaging.h"
#include <QThread>

Packaging::Packaging(QObject* parent)
    : WorkStation("Packaging", parent)
{
    setProcessingTime(180, 350); // Packaging takes moderate time
    setFailureRate(0.01); // 1% failure rate - packaging damage
}

bool Packaging::processProduct(std::shared_ptr<Product> product)
{
    // Check for packaging failure
    if (shouldRejectProduct()) {
        logActivity(QString("Packaging failed for product %1 - damaged during packaging").arg(product->getId()));
        product->setState(ProductState::Rejected);
        return false;
    }
    
    // Get packaging specifications
    PackageSpec spec = getPackageSpec(product->getType());
    
    logActivity(QString("Starting packaging for product %1 with %2")
                .arg(product->getId(), spec.boxType));
    
    // Perform packaging steps
    performPackagingStep("Prepare packaging materials", product);
    performPackagingStep("Place product in protective materials", product);
    performPackagingStep("Add accessories and documentation", product);
    performPackagingStep("Seal and label package", product);
    performPackagingStep("Quality check package integrity", product);
    
    // Update product state
    product->setState(ProductState::AtPackaging);
    product->addTraceEntry("Packaging");
    product->advanceState();
    
    logActivity(QString("Successfully packaged product %1 (estimated weight: %2 kg)")
                .arg(product->getId()).arg(spec.estimatedWeight));
    
    return true;
}

Packaging::PackageSpec Packaging::getPackageSpec(ProductType type) const
{
    PackageSpec spec;
    
    switch (type) {
        case ProductType::Washer:
            spec.boxType = "Heavy-duty cardboard box with foam inserts";
            spec.materials = {"Foam padding", "Plastic wrap", "Cardboard reinforcement"};
            spec.accessories = {"User manual", "Warranty card", "Installation kit", "Hoses"};
            spec.estimatedWeight = 75;
            break;
            
        case ProductType::Dryer:
            spec.boxType = "Standard appliance box with corner protection";
            spec.materials = {"Corner protectors", "Plastic wrap", "Foam inserts"};
            spec.accessories = {"User manual", "Warranty card", "Vent kit", "Power cord"};
            spec.estimatedWeight = 68;
            break;
            
        case ProductType::Refrigerator:
            spec.boxType = "Extra-large appliance box with strapping";
            spec.materials = {"Heavy foam padding", "Plastic wrap", "Strapping bands"};
            spec.accessories = {"User manual", "Warranty card", "Ice maker kit", "Shelves"};
            spec.estimatedWeight = 125;
            break;
            
        case ProductType::Dishwasher:
            spec.boxType = "Medium appliance box with protective wrap";
            spec.materials = {"Bubble wrap", "Foam corners", "Plastic covering"};
            spec.accessories = {"User manual", "Warranty card", "Installation kit", "Dish racks"};
            spec.estimatedWeight = 58;
            break;
            
        case ProductType::Oven:
            spec.boxType = "Reinforced appliance box with thermal protection";
            spec.materials = {"Thermal padding", "Protective wrap", "Corner guards"};
            spec.accessories = {"User manual", "Warranty card", "Oven racks", "Baking tray"};
            spec.estimatedWeight = 82;
            break;
    }
    
    return spec;
}

void Packaging::performPackagingStep(const QString& step, std::shared_ptr<Product> product)
{
    logActivity(QString("Packaging step: %1 for product %2").arg(step, product->getId()));
    
    // Simulate step processing time
    QThread::msleep(getRandomProcessingTime() / 5); // Distribute time across steps
}
