/** -*- c++ -*-
 * Copyright (C) 2007-2012 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 3 of the
 * License, or any later version.
 *
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef HYPERTABLE_OPERATIONLOADBALANCER_H
#define HYPERTABLE_OPERATIONLOADBALANCER_H

#include "Operation.h"
#include "Hypertable/Lib/BalancePlan.h"

namespace Hypertable {

  class OperationLoadBalancer : public Operation {
  public:
    OperationLoadBalancer(ContextPtr &context);
    OperationLoadBalancer(ContextPtr &context, EventPtr &event);
    virtual ~OperationLoadBalancer() { }

    virtual void execute();
    virtual const String name();
    virtual const String label();
    virtual bool exclusive() { return true; }

    virtual void display_state(std::ostream &os) { }
    virtual size_t encoded_state_length() const { return 0; }
    virtual void encode_state(uint8_t **bufp) const { }
    virtual void decode_state(const uint8_t **bufp, size_t *remainp) { }
    virtual void decode_request(const uint8_t **bufp, size_t *remainp);
    const String get_algorithm();

  private:
    void initialize_dependencies();
    BalancePlanPtr m_plan;
  };
  typedef intrusive_ptr<OperationLoadBalancer> OperationLoadBalancerPtr;

} // namespace Hypertable

#endif // HYPERTABLE_OPERATIONLOADBALANCER_H
