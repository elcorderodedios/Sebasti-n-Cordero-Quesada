#ifndef PACKAGING_H
#define PACKAGING_H

#include "../WorkStation.h"

class Packaging : public WorkStation
{
    Q_OBJECT

public:
    explicit Packaging(QObject* parent = nullptr);

protected:
    bool processProduct(std::shared_ptr<Product> product) override;

private:
    struct PackageSpec {
        QString boxType;
        QStringList materials;
        QStringList accessories;
        int estimatedWeight;
    };
    
    PackageSpec getPackageSpec(ProductType type) const;
    void performPackagingStep(const QString& step, std::shared_ptr<Product> product);
};

#endif // PACKAGING_H
