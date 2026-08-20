#ifndef OC_HTTP_PRODUCT_ROUTES_H
#define OC_HTTP_PRODUCT_ROUTES_H

#include "oc/http/router.h"

namespace oc::http {

// Product HTTP apps register routes on an HttpRouter. Platform routing does not
// hard-code product paths; adding GET /api/v1/ping is a Register() call.
class ProductRoutes {
  public:
    virtual ~ProductRoutes() = default;
    virtual void Register(HttpRouter &router) = 0;
};

} // namespace oc::http

#endif
