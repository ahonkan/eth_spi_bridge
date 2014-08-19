class DriverAutoEnableTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @driver_auto_enable_list = Hash.new(0)
      @debug = false
   end

   def executePhase(context)
        @context = context
        context.getUniverse().accept(self)

        # auto-enable/disable drivers based on their reference count
        @driver_auto_enable_list.each { |key,object|

            # Check if reference count is > 0
            if object.getIntegerProperty("ref_count") > 0
                # Enable this driver
                object.setEnabled(true)
            else
                # Ref count is 0 - disable the driver
                object.setEnabled(false)
            end

            if @debug
                # Output reference count for each driver
                printf("Driver name: %s\n", object.getPackageName() + "." + object.getName())
                printf("Ref count:   %d\n\n", object.getIntegerProperty("ref_count"))
            end
        }

      return true
   end

   def preVisitComponent(obj)

        # Determine if this component is a device instance fromt the platform file
        # by checking if it has a "driver" property and the package name is the platform name
        if obj.hasProperty("driver") and @context.getPlatformName() == obj.getPackageName()

            # Get referenced driver name
            driver_name = obj.getStringProperty("driver")

            # Get the driver object using the fully-qualified name in the device instance node
            driver_obj = @context.getUniverse().findFQComponent(driver_name)

            # Check if this component / device is disabled
            if obj.getEnabled() == false

                # Get the driver's current reference count
                ref_count = driver_obj.getIntegerProperty("ref_count")

                # Decrement the driver reference count
                ref_count -= 1

                # Save updated reference count for this driver
                driver_obj.setIntegerProperty("ref_count", ref_count)

            end

            # Save driver name and driver object in hash so the driver can
            # be appropriately auto-enabled/disabled later
            @driver_auto_enable_list[driver_name] = driver_obj

        end
    end
end
