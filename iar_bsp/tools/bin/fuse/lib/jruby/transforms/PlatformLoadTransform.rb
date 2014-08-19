require 'rbconfig'
require 'platform-dsl'

class PlatformLoadTransform < Transform
   def initialize(phaseList)
   end
   
   def executePhase(context)
      build_root = context.getBuildRoot().getAbsolutePath()
      platform_defs_dir = File.join(build_root, 'bsp', context.getPlatformName)
 
      # Check to see if we have an platform definitions. 
      if File.exist?(platform_defs_dir)
         # Apply each platform definition file.
         Dir.foreach(platform_defs_dir) { |fname|
            if fname =~ /^(.+)\.platform$/ and $1 == context.getPlatformName
               abs_fname = File.join(platform_defs_dir, fname)
               if PlatformDSL.load(abs_fname, context.getUniverse()) == nil
                  return false
               end
            end
         }
      end

      return true
   end
end
