/*
  ==============================================================================
    FastBus.h
    Created: September 2025
    Author:  Optimized MessageBus Implementation

    A fast, header-only message bus system for pub/sub pattern implementation.
    Supports any standard data type including juce::String.

    Usage:
      // Get singleton instance
      auto& bus = FastBus::getInstance();

      // Subscribe to topic changes
      bus.subscribe<float>("volume", [](const FastTopic<float>& topic) {
          std::cout << "Volume changed to: " << topic.getValue() << std::endl;
      });

      // Publish value changes
      bus.publish("volume", 0.75f);

      // Get topic directly
      auto& volumeTopic = bus.getTopic<float>("volume");

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <typeindex>

//==============================================================================
/**
 * @brief Generic topic container that holds a typed value and notifies listeners on changes
 * @tparam T The data type stored in this topic
 */
template<typename T>
class FastTopic
{
public:
    using ValueType = T;
    using ListenerCallback = std::function<void(const FastTopic<T>&)>;

    /**
     * @brief Constructs a new FastTopic with the given name
     * @param topicName The name identifier for this topic
     */
    explicit FastTopic(const juce::String& topicName)
        : name(topicName), value{} {
    }

    /**
     * @brief Constructs a new FastTopic with name and initial value
     * @param topicName The name identifier for this topic
     * @param initialValue The initial value to set
     */
    FastTopic(const juce::String& topicName, const T& initialValue)
        : name(topicName), value(initialValue) {
    }

    // Non-copyable but movable for performance
    FastTopic(const FastTopic&) = delete;
    FastTopic& operator=(const FastTopic&) = delete;
    FastTopic(FastTopic&&) = default;
    FastTopic& operator=(FastTopic&&) = default;

    /**
     * @brief Gets the topic name
     * @return The topic name as juce::String
     */
    const juce::String& getName() const noexcept { return name; }

    /**
     * @brief Gets the current value
     * @return Const reference to the stored value
     */
    const T& getValue() const noexcept { return value; }

    /**
     * @brief Sets a new value and notifies all listeners if the value changed
     * @param newValue The new value to set
     * @return true if the value was changed and listeners were notified
     */
    bool setValue(const T& newValue)
    {
        if constexpr (std::is_arithmetic_v<T>) {
            // For arithmetic types, use direct comparison
            if (value != newValue) {
                value = newValue;
                notifyListeners();
                return true;
            }
        }
        else {
            // For complex types, always update and notify
            value = newValue;
            notifyListeners();
            return true;
        }
        return false;
    }

    /**
     * @brief Adds a listener callback that gets called when the value changes
     * @param callback Function to call on value changes
     */
    void addListener(ListenerCallback callback)
    {
        listeners.emplace_back(std::move(callback));
    }

    /**
     * @brief Gets the number of registered listeners
     * @return Number of listeners
     */
    size_t getListenerCount() const noexcept { return listeners.size(); }

    /**
     * @brief Clears all listeners
     */
    void clearListeners() { listeners.clear(); }

private:
    juce::String name;
    T value;
    std::vector<ListenerCallback> listeners;

    /**
     * @brief Notifies all registered listeners about value change
     */
    void notifyListeners()
    {
        for (const auto& listener : listeners) {
            listener(*this);
        }
    }
};

//==============================================================================
/**
 * @brief Base class for type-erased topic storage
 */
class TopicBase
{
public:
    virtual ~TopicBase() = default;
    virtual const juce::String& getName() const = 0;
    virtual std::type_index getTypeIndex() const = 0;
};

/**
 * @brief Type-erased wrapper for FastTopic instances
 */
template<typename T>
class TopicWrapper : public TopicBase
{
public:
    explicit TopicWrapper(const juce::String& name) : topic(name) {}
    TopicWrapper(const juce::String& name, const T& initialValue) : topic(name, initialValue) {}

    const juce::String& getName() const override { return topic.getName(); }
    std::type_index getTypeIndex() const override { return std::type_index(typeid(T)); }

    FastTopic<T> topic;
};

//==============================================================================
/**
 * @brief Fast, type-safe message bus implementation using singleton pattern
 *
 * This class provides a high-performance publish/subscribe system that supports
 * any standard data type. Topics are automatically created when first accessed.
 * The implementation uses templates and type erasure for optimal performance
 * while maintaining type safety.
 */
class FastBus
{
public:
    /**
     * @brief Gets the singleton instance of FastBus
     * @return Reference to the singleton instance
     */
    static FastBus& getInstance()
    {
        static FastBus instance;
        return instance;
    }

    /**
     * @brief Gets or creates a typed topic
     * @tparam T The data type for the topic
     * @param topicName The name of the topic
     * @return Reference to the FastTopic instance
     */
    template<typename T>
    FastTopic<T>& getTopic(const juce::String& topicName)
    {
        const auto key = createKey<T>(topicName);

        auto it = topics.find(key);
        if (it != topics.end()) {
            auto* wrapper = static_cast<TopicWrapper<T>*>(it->second.get());
            return wrapper->topic;
        }

        auto wrapper = std::make_unique<TopicWrapper<T>>(topicName);
        auto& topicRef = wrapper->topic;
        topics[key] = std::move(wrapper);

        return topicRef;
    }

    /**
     * @brief Gets or creates a typed topic with initial value
     * @tparam T The data type for the topic
     * @param topicName The name of the topic
     * @param initialValue Initial value if topic doesn't exist
     * @return Reference to the FastTopic instance
     */
    template<typename T>
    FastTopic<T>& getTopic(const juce::String& topicName, const T& initialValue)
    {
        const auto key = createKey<T>(topicName);

        auto it = topics.find(key);
        if (it != topics.end()) {
            auto* wrapper = static_cast<TopicWrapper<T>*>(it->second.get());
            return wrapper->topic;
        }

        auto wrapper = std::make_unique<TopicWrapper<T>>(topicName, initialValue);
        auto& topicRef = wrapper->topic;
        topics[key] = std::move(wrapper);

        return topicRef;
    }

    /**
     * @brief Publishes a value to a topic, creating it if necessary
     * @tparam T The data type for the topic
     * @param topicName The name of the topic
     * @param value The value to publish
     * @return true if the value was changed and listeners were notified
     */
    template<typename T>
    bool publish(const juce::String& topicName, const T& value)
    {
        auto& topic = getTopic<T>(topicName);
        return topic.setValue(value);
    }

    /**
     * @brief Subscribes to topic changes with a callback
     * @tparam T The data type for the topic
     * @param topicName The name of the topic to subscribe to
     * @param callback Function to call when topic value changes
     */
    template<typename T>
    void subscribe(const juce::String& topicName, typename FastTopic<T>::ListenerCallback callback)
    {
        auto& topic = getTopic<T>(topicName);
        topic.addListener(std::move(callback));
    }

    /**
     * @brief Checks if a topic exists
     * @tparam T The data type for the topic
     * @param topicName The name of the topic
     * @return true if the topic exists
     */
    template<typename T>
    bool hasTopic(const juce::String& topicName) const
    {
        const auto key = createKey<T>(topicName);
        return topics.find(key) != topics.end();
    }

    /**
     * @brief Removes a topic and all its listeners
     * @tparam T The data type for the topic
     * @param topicName The name of the topic to remove
     * @return true if the topic was found and removed
     */
    template<typename T>
    bool removeTopic(const juce::String& topicName)
    {
        const auto key = createKey<T>(topicName);
        return topics.erase(key) > 0;
    }

    /**
     * @brief Gets all topic names (regardless of type)
     * @return StringArray containing all topic names
     */
    juce::StringArray getAllTopicNames() const
    {
        juce::StringArray names;
        for (const auto& pair : topics) {
            names.add(pair.second->getName());
        }
        return names;
    }

    /**
     * @brief Gets the total number of topics
     * @return Number of topics currently stored
     */
    size_t getTopicCount() const noexcept { return topics.size(); }

    /**
     * @brief Clears all topics and their listeners
     */
    void clear() { topics.clear(); }

private:
    // Private constructor for singleton
    FastBus() = default;

    // Non-copyable and non-movable
    FastBus(const FastBus&) = delete;
    FastBus& operator=(const FastBus&) = delete;
    FastBus(FastBus&&) = delete;
    FastBus& operator=(FastBus&&) = delete;

    /**
     * @brief Creates a unique key for topic storage based on name and type
     * @tparam T The data type
     * @param topicName The topic name
     * @return Unique string key for the topic
     */
    template<typename T>
    std::string createKey(const juce::String& topicName) const
    {
        return topicName.toStdString() + "_" + typeid(T).name();
    }

    // Storage for all topics using type erasure
    std::unordered_map<std::string, std::unique_ptr<TopicBase>> topics;
};

//==============================================================================
// Convenience macros for common usage patterns

/**
 * @brief Macro to easily get the FastBus singleton
 */
#define FASTBUS FastBus::getInstance()

 /**
  * @brief Macro to quickly publish a value
  * @param topic_name The name of the topic
  * @param value The value to publish
  */
#define FASTBUS_PUBLISH(topic_name, value) \
    FastBus::getInstance().publish(topic_name, value)

  /**
   * @brief Macro to quickly subscribe to a topic
   * @param type The data type of the topic
   * @param topic_name The name of the topic
   * @param callback The callback function
   */
#define FASTBUS_SUBSCRIBE(type, topic_name, callback) \
    FastBus::getInstance().subscribe<type>(topic_name, callback)

   //==============================================================================
   // Example usage in comments:
   /*
       // Basic usage
       auto& bus = FastBus::getInstance();

       // Subscribe to float topic
       bus.subscribe<float>("volume", [](const FastTopic<float>& topic) {
           DBG("Volume changed to: " << topic.getValue());
       });

       // Subscribe to juce::String topic
       bus.subscribe<juce::String>("status", [](const FastTopic<juce::String>& topic) {
           DBG("Status: " << topic.getValue());
       });

       // Publish values
       bus.publish("volume", 0.75f);
       bus.publish("status", juce::String("Ready"));

       // Get topic directly for multiple operations
       auto& volumeTopic = bus.getTopic<float>("volume");
       volumeTopic.setValue(0.5f);
       float currentVolume = volumeTopic.getValue();

       // Using macros
       FASTBUS_PUBLISH("gain", 1.2f);
       FASTBUS_SUBSCRIBE(int, "buffer_size", [](const FastTopic<int>& topic) {
           DBG("Buffer size: " << topic.getValue());
       });
   */