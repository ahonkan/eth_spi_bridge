module BuildProperties
   def isToolsetPackage?(p)
      p.getName() == "toolsets"
   end

   def outputDirectory()    
      
      return File.join("output",
         @context.getUniverse().getStringProperty("toolset"),
         @context.getUniverse().getStringProperty("platform"),
         File.basename(@context.getUniverse().getStringProperty("config")))
   end
  
   def isSpecificToolsetPackage?(p)
      (p.getName() == @context.getPlatformName()) \
         and isToolsetPackage?(p.getParent())
   end

   def shouldMakeComponent?(c)
      suitable_platform = c.getPlatforms().length == 0 
      suitable_platform ||= c.supportsPlatform(@context.getPlatformName())
	  suitable_toolset = c.getToolsetName() == nil 
      suitable_toolset ||= (c.getToolsetName() == @context.getToolsetName())
      suitable_architecture = c.getArchitecture() == nil
      suitable_architecture ||= (c.getArchitecture() == @context.getUniverse().getStringProperty("architecture"))
	  suitable_platform && suitable_toolset && suitable_architecture && c.getEnabled()
   end
  
   def isInToolsetPackage?(o)
      f = o.getDirectory()
      f != nil and f.getAbsolutePath().index("toolsets") != nil
   end
end

