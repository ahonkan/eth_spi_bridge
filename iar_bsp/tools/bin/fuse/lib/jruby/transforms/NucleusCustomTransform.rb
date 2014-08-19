# Transform for performing Nucleus specific
# changes to the model. 
#
# 
class NucleusCustomException < Exception
end

class NucleusCustomTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @namespace = []
   end
   
   def executePhase(context)
      @context = context
      begin 
         # test the state of "nu.os.svcs.pwr.core.set_def_dev_power_state_off"
         key = "nu.os.svcs.pwr.core.set_def_dev_power_state_off"
         if @context.universe.hasProperty(key)
            if @context.universe.getBooleanProperty(key)
               # key is true
               set_each_device_power_state_off
            end
         end
         return true
 
      rescue NucleusCustomException => e
         puts "error: " + e.message
      end
      return false
   end

   # Set each devices' power state to 0.The default device power state is 
   # kept here: {platform}.{device_name}.tgt_settings.def_pwr_state
   # where platform is the selected platform that exists in the model as a 
   # package. Each component within platform package is tested for the 
   # correct property prior to setting. 
   def set_each_device_power_state_off
      selected_platform = @context.getPlatformName()
      @bsp = @context.universe.findPackage(selected_platform)
      if @bsp != nil
         @bsp.getComponents.each { |device_component|
            device_name = device_component.getName()  
            device_power_key = "#{selected_platform}.#{device_name}.tgt_settings.def_pwr_state"
            if @context.universe.hasProperty(device_power_key)
              @context.universe.setProperty(device_power_key, 0)
            end
         }
      end
   end
end

