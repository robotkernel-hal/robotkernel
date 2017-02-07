//
// Created by crem_ja on 2/6/17.
//

#ifndef PROJECT_COMMBRIDGE_H
#define PROJECT_COMMBRIDGE_H

namespace robotkernel {
    
    class CommBridgeInterface{
        
        //! create and register ln service
        /*!
         * \param svc robotkernel service struct
         */
        virtual void addService(const robotkernel::service_t &svc) = 0;

        //! unregister and remove ln service 
        /*!
         * \param svc robotkernel service struct
         */
        virtual void removeService(const robotkernel::service_t &svc) = 0;
    };
}

#endif //PROJECT_COMMBRIDGE_H
