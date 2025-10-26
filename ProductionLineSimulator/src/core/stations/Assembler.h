#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "../WorkStation.h"

class Assembler : public WorkStation
{
    Q_OBJECT

public:
    explicit Assembler(QObject* parent = nullptr);

protected:
    bool processProduct(std::shared_ptr<Product> product) override;

private:
    void performAssemblyStep(std::shared_ptr<Product> product, const QString& step);
    QStringList getAssemblySteps(ProductType type) const;
};

#endif // ASSEMBLER_H
