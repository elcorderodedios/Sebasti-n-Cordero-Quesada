#ifndef QUALITYINSPECTION_H
#define QUALITYINSPECTION_H

#include "../WorkStation.h"

class QualityInspection : public WorkStation
{
    Q_OBJECT

public:
    explicit QualityInspection(QObject* parent = nullptr);

protected:
    bool processProduct(std::shared_ptr<Product> product) override;

private:
    struct TestResult {
        QString testName;
        bool passed;
        QString details;
    };
    
    QList<TestResult> performQualityTests(std::shared_ptr<Product> product);
    bool shouldSendToRework(const QList<TestResult>& results) const;
    
    double m_reworkRate; // Rate of products sent back for rework
};

#endif // QUALITYINSPECTION_H
