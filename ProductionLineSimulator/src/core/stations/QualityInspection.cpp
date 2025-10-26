#include "QualityInspection.h"
#include <QThread>
#include <QRandomGenerator>

QualityInspection::QualityInspection(QObject* parent)
    : WorkStation("Quality Inspection", parent)
    , m_reworkRate(0.08) // 8% rework rate
{
    setProcessingTime(150, 300); // Inspection takes time
    setFailureRate(0.03); // 3% complete rejection rate
}

bool QualityInspection::processProduct(std::shared_ptr<Product> product)
{
    // Perform quality tests
    QList<TestResult> results = performQualityTests(product);
    
    // Check for complete failure
    if (shouldRejectProduct()) {
        product->setState(ProductState::Rejected);
        logActivity(QString("Product %1 failed quality inspection - REJECTED").arg(product->getId()));
        return false;
    }
    
    // Check if product needs rework
    if (shouldSendToRework(results)) {
        product->setReworkFlag(true);
        product->setState(ProductState::InRework);
        product->addTraceEntry("Quality Inspection - Rework Required");
        logActivity(QString("Product %1 requires rework").arg(product->getId()));
        
        // Send back to assembler via special handling
        product->advanceState(); // This will send it back to assembler
        return true;
    }
    
    // Product passes inspection
    product->setState(ProductState::AtQualityInspection);
    product->addTraceEntry("Quality Inspection - Passed");
    product->advanceState();
    
    int passedTests = 0;
    for (const auto& result : results) {
        if (result.passed) passedTests++;
    }
    
    logActivity(QString("Product %1 passed quality inspection (%2/%3 tests passed)")
                .arg(product->getId()).arg(passedTests).arg(results.size()));
    
    return true;
}

QList<QualityInspection::TestResult> QualityInspection::performQualityTests(std::shared_ptr<Product> product)
{
    QList<TestResult> results;
    
    // Simulate inspection time
    QThread::msleep(getRandomProcessingTime());
    
    // Define tests based on product type
    QStringList testNames;
    switch (product->getType()) {
        case ProductType::Washer:
            testNames = {"Water seal test", "Motor function test", "Control panel test", "Drum alignment test"};
            break;
        case ProductType::Dryer:
            testNames = {"Heating test", "Ventilation test", "Control panel test", "Safety interlock test"};
            break;
        case ProductType::Refrigerator:
            testNames = {"Cooling test", "Insulation test", "Door seal test", "Temperature control test"};
            break;
        case ProductType::Dishwasher:
            testNames = {"Water pressure test", "Spray pattern test", "Control panel test", "Drainage test"};
            break;
        case ProductType::Oven:
            testNames = {"Heating uniformity test", "Insulation test", "Control panel test", "Safety test"};
            break;
    }
    
    // Perform each test with random outcomes
    for (const QString& testName : testNames) {
        TestResult result;
        result.testName = testName;
        result.passed = QRandomGenerator::global()->generateDouble() > 0.15; // 85% pass rate per test
        result.details = result.passed ? "PASS" : "FAIL - Minor defect";
        results.append(result);
        
        // Small delay between tests
        QThread::msleep(10);
    }
    
    return results;
}

bool QualityInspection::shouldSendToRework(const QList<TestResult>& results) const
{
    // Count failed tests
    int failedTests = 0;
    for (const auto& result : results) {
        if (!result.passed) failedTests++;
    }
    
    // If more than 1 test failed, or random rework chance
    if (failedTests > 1) {
        return true;
    }
    
    if (failedTests == 1) {
        return QRandomGenerator::global()->generateDouble() < m_reworkRate;
    }
    
    return false;
}
