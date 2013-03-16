dir = File.dirname(__FILE__)
$:.unshift dir
require 'digiusb'

spark = DigiUSB.sparks.last

TimeoutRepeatBlocker = 2.5 # seconds till will accept the same code again as a second press

# Read a string fromt he Digispark until a newline is received (eg, from the println function in Digispark's DigiUSB library)
# The returned string includes a newline character on the end.
class DigiUSB
  PollingFrequency = 5

  def gets
    chars = ""
    until chars.include? "\n"
      char = getc()
      chars += char
      sleep 1.0 / PollingFrequency if char == ""
    end
    return chars
  end
end

puts "ready to listen..."
last_code = nil
last_code_timestamp = nil
loop do
  code = spark.gets.strip
  puts "received #{code}"
  
  if code != last_code || Time.now - last_code_timestamp > TimeoutRepeatBlocker
    Thread.new do
      if File.exists? "#{dir}/scripts/#{code}"
        puts "running script/#{code} as executable"
        print `'#{dir}/scripts/#{code}'`
      elsif File.exists? "#{dir}/scripts/#{code}.rb"
        puts "running script/#{code}.rb in ruby"
        Process.fork do
          load "#{dir}/scripts/#{code}.rb"
          exit
        end
      elsif File.exists? "#{dir}/scripts/#{code}.sh"
        puts "running script/#{code}.sh in bash"
        print `bash '#{dir}/scripts/#{code}.sh'`
      end
    end
  end
  last_code = code
  last_code_timestamp = Time.now
end
