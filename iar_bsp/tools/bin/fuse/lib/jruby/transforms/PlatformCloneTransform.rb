require 'find'
require 'fileutils'
include FileUtils

class PlatformCloneTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @depth = 0
      @namespace = []
      @platRoot = nil
      @cBSPSource="\\bsp"
   end
   
   def executePhase(context)
      @context = context

      platformName = context.getPlatformName()
      puts "Platform = #{platformName}"

      cloneName = context.getUniverse().getStringProperty("clone")
      puts "Clone = #{cloneName}"
      
      bp = context.getUniverse().findFQPackage("#{platformName}")
      bspPath = bp.getDirectory()

      @selected_drivers = []
      @device_name = {}

      ##################################################
      # Copy all non-driver files from /bsp/<PLATFORM> #
      ##################################################
      srcBspFiles = []
      srcBspFiles = matchFiles(File.join("#{bspPath}","*.*"))
      srcBspFiles1 = []
      srcBspFiles1 = matchDirs(File.join("#{bspPath}","arch"))
      srcBspFiles2 = []
      srcBspFiles2 = matchDirs(File.join("#{bspPath}","config"))
      srcBspFiles3 = []
      srcBspFiles3 = matchFiles(File.join("#{bspPath}","include","bsp","*.*"))
      srcBspFiles4 = []
      srcBspFiles4 = matchDirs(File.join("#{bspPath}","include","bsp","arch"))
      srcBspFiles5 = []
      srcBspFiles5 = matchDirs(File.join("#{bspPath}","toolset"))

      # files to process are in: srcBspFilesn
      list = [srcBspFiles,srcBspFiles1,srcBspFiles2,srcBspFiles3,srcBspFiles4,srcBspFiles5]
      singleSrcList = []
      list.each do |l|
         l.each do |file|
             # unify the path separator 
             singleSrcList << file.gsub(/\\/,'/')
          end
       end

      # create a list of files to copy
	  fileMap = {}
      singleSrcList.each do |src|
         fileMap[src] = src.gsub(/(#{platformName})/,"#{cloneName}")
	  end

     # make the dir structure
     fileMap.each do |key,value|
        if File.directory?(value) == false    
           FileUtils.mkdir_p File.dirname(value)
        end   
     end

     fileMap.each do |key,value|
        f = File.open("#{key}") 
        inData = f.read()
        inData.gsub!(/#{platformName}/,"#{cloneName}")
        inData.gsub!(/#{platformName.upcase}/,"#{cloneName.upcase}")
        f.close()
        f = File.open("#{value}","w")
        f.puts(inData)
        f.close()
     end
    
     ########################################################################
     # Copy target support files from /bsp/<PLATFORM> without altering them #
     ########################################################################
     srcBspFiles = []
     srcBspFiles = matchDirs(File.join("#{bspPath}","target_support"))

      # files to process are in: srcBspFiles
      list = [srcBspFiles]
      singleSrcList = []
      list.each do |l|
         l.each do |file|
             # unify the path separator 
             singleSrcList << file.gsub(/\\/,'/')
          end
       end

      # create a list of files to copy
	  fileMap = {}
      singleSrcList.each do |src|
         fileMap[src] = src.gsub(/(bsp\/#{platformName})/,"bsp\/#{cloneName}")
	  end

     # make the dir structure
     fileMap.each do |key,value|
        if File.directory?(value) == false    
           FileUtils.mkdir_p File.dirname(value)
        end   
     end

     fileMap.each do |key,value|
        f = File.open("#{key}") 
        inData = f.read()
        f.close()
        f = File.open("#{value}","w")
        f.puts(inData)
        f.close()
     end
    
    ######################################################
    # Copy "requested" driver files from /bsp/<PLATFORM> #
    ######################################################
    srcDrvDirs=[]
    getDrivers("nu.bsp.drvr",srcDrvDirs)

      # files to process are in: filelist
      singleSrcList = []
      srcDrvDirs.each do |l|
         filelist = matchDirs(l)
         filelist.each do |file|
             # unify the path separator 
             singleSrcList << file.gsub(/\\/,'/')
          end
       end

      # create a list of files to copy
      fileMap = {}
      singleSrcList.each do |src|
         fileMap[src] = src.gsub(/(#{platformName})/,"#{cloneName}")
      end

     # make the dir structure
     fileMap.each do |key,value|
        if File.directory?(value) == false    
           FileUtils.mkdir_p File.dirname(value)
        end   
     end

     fileMap.each do |key,value|
        if File.exists?("#{key}")
            f = File.open("#{key}") 
            inData = f.read()
            inData.gsub!(/#{platformName}/,"#{cloneName}")
            inData.gsub!(/#{platformName.upcase}/,"#{cloneName.upcase}")
            f.close()
            f = File.open("#{value}","w")
            f.puts(inData)
            f.close()
        end
     end
    
     ##########################################################################
     # Remove deleted driver devices from /bsp/<PLATFORM_CLONE>.platform file #
     # Copy "requested" device instance files from /bsp/<PLATFORM>            #
     ##########################################################################
     cloneBspPath = "#{bspPath}".gsub!(/#{platformName}/,"#{cloneName}")
     inData = []
     tempData = []
     outData = []
     devData = []
     nested = 0
     keep_bsp_driver = false
     found_bsp_driver = false
     check_for_blank_line = false

     f = File.open(File.join("#{cloneBspPath}","#{cloneName}.platform"),"r")
     inData = f.read()
     inData.each do |line|
         if nested == 0
            if line.include? "device("
                @device_name = line.strip.slice((line.strip.index("device(\"")+"device(\"".length)..line.strip.length)
                @device_name = @device_name.slice(0..(@device_name.index("\"")-1))
                if line.include? "{"
                    nested = nested + 1
                    tempData = line
                end
            else
                if check_for_blank_line == true
                    if line.strip.length > 0
                        outData << line
                    end
                    check_for_blank_line = false
                else
                    outData << line
                end
            end
        else
            if line.include? "{"
                nested = nested + 1
            end
            if line.include? "}"
                nested = nested - 1
            end

            if line.include? "nu.bsp.drvr."
                @selected_drivers.each do |driver|
                    if line.include? "#{driver}"
                        keep_bsp_driver = true
                    end
                end
                found_bsp_driver = true
            end

            if line.include? "nu.os.drvr."

                # Ask about OS driver files
                drv_name = line.strip
                index = "#{drv_name}".index("drvr.") + "drvr.".length
                length = "#{drv_name}".length-2
                drv_name = "#{drv_name}".slice(index..length)
                drv_name = drv_name.gsub(/(\.)/," ")
                print("Would you like to include the #{drv_name.upcase} driver (y/n): ")
                answer = gets.chomp
                if answer.upcase == "Y"
                    keep_bsp_driver = true
                end
                found_bsp_driver = true
            end

            tempData << line

            if nested == 0
                if found_bsp_driver == false
                    outData << tempData
                else
                    if keep_bsp_driver == true
                        outData << tempData
                        keep_bsp_driver = false
                        
                        # Create "requested" device instance file
                        if File.exists?(File.join("#{bspPath}","devices","#{@device_name}.c"))
                            if File.directory?(File.join("#{cloneBspPath}","devices")) == false    
                                FileUtils.mkdir_p File.join("#{cloneBspPath}","devices")
                            end
                            df = File.open(File.join("#{bspPath}","devices","#{@device_name}.c")) 
                            devData = df.read()
                            devData.gsub!(/#{platformName}/,"#{cloneName}")
                            devData.gsub!(/#{platformName.upcase}/,"#{cloneName.upcase}")
                            df.close()
                            df = File.open(File.join("#{cloneBspPath}","devices","#{@device_name}.c"),"w")
                            df.puts(devData)
                            df.close()
                        end
                    else
                        check_for_blank_line = true
                    end
                    found_bsp_driver = false
                end
            end
        end
     end
     f.close()

     # Create /bsp/<PLATFORM_CLONE>.platform file
     f = File.open(File.join("#{cloneBspPath}","#{cloneName}.platform"),"w")
     f.puts(outData)
     f.close()

     return true
   end

   def matchDirs(srcExpression)
      matchList = []
      Find.find("#{srcExpression}") do |path|
         if FileTest.directory?(path)
            next
         else
            matchList << path
         end
      end
      return matchList
   end

   def matchFiles(srcExpression)
      matchList = []
      Dir["#{srcExpression}"].each do |path|
         matchList << path
      end
      return matchList
   end

    def getDrivers(path,name)
        rootPack = @context.getUniverse().findFQPackage(path)
        if rootPack != nil
            # Vist child components
            comps = rootPack.getComponents()
            comps.each do |cp|

                # Ask about BSP driver files
                src_name = cp.getDirectory()
                src_name = "#{src_name}".gsub(/\\/,'/')
                if src_name.index("samples") != nil
                    next
                end
                index = "#{src_name}".index("drivers\/") + "drivers\/".length
                length = "#{src_name}".length
                drv_name = "#{src_name}".slice(index,length)
                drv_name = drv_name.gsub(/(\/)/," ")
                print("Would you like to include the #{drv_name.upcase} driver (y/n): ")
                answer = gets.chomp
                if answer.upcase == "Y"
                    @selected_drivers << "#{path}.#{cp.getName()}"
                    name << src_name
                    inc_name = src_name
                    inc_name = inc_name.gsub(/(#{@context.getPlatformName()}\/drivers)/,"#{@context.getPlatformName}\/include\/bsp\/drivers")
                    name << inc_name
                end
            end

            # Vist child packages if any
            pkgs = rootPack.getPackages()
            if pkgs.length != 0
                pkgs.each do |pp|
                    temp_name = pp.getName()
                    temp_path = "#{path}.#{temp_name}"
                    getDrivers(temp_path,name)
                end
            end
        end
    end
end
